// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sqlitedb.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <set>
#include <algorithm>
#include <functional>
#include <openssl/sha.h>
#include <glm/gtc/type_ptr.hpp>

SQLiteDB::SQLiteDB(const std::string& name) :
	m_db(nullptr)
{
	struct passwd* pw = getpwuid(getuid());
	const std::string home_dir = pw->pw_dir;
	const std::string db_file = home_dir + "/.config/" + name + ".sqlite";

	int status = sqlite3_open(db_file.c_str(), &m_db);

	if (status)
	{
		throw std::runtime_error(std::string("Error open DB ") + sqlite3_errmsg(m_db));
	}

	update_database();
}

SQLiteDB::~SQLiteDB(void)
{
	if (m_db)
	{
		sqlite3_close(m_db);
		m_db = nullptr;
	}
}

std::string SQLiteDB::checksum_file(const std::string& path) const
{
	const size_t max_hash_length = 0x100000;    /* 1MiB */

	std::ifstream in_file(path, std::ios::in | std::ios::binary);

	in_file.seekg(0, std::ios::end);
	const size_t file_size = static_cast<size_t>(in_file.tellg());
	const size_t hash_length = std::min<size_t>(max_hash_length, file_size);
	const size_t file_offset = (file_size - hash_length) / 2;
	in_file.seekg(static_cast<std::streamsize>(file_offset), std::ios::beg);
	std::vector<char> buffer(hash_length);
	in_file.read(&buffer[0], static_cast<std::streamsize>(hash_length));
	in_file.close();

	std::vector<unsigned char> hash_bytes(SHA256_DIGEST_LENGTH);
	SHA256(reinterpret_cast<unsigned char*>(&buffer[0]), hash_length, &hash_bytes[0]);

	std::stringstream s;
	for (size_t i = 0; i < hash_bytes.size(); i++)
	{
		s << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(hash_bytes[i]);
	}

	std::string hash = s.str();

	return hash;
}

void SQLiteDB::run_command(const std::string& command) const
{
	char* messageError;
	const int status = sqlite3_exec(m_db, command.c_str(), nullptr, nullptr, &messageError);

	if (status != SQLITE_OK)
	{
		std::string message = messageError;
		sqlite3_free(messageError);
		throw std::runtime_error(std::string("Error executing command: ") + message);
	}
}

size_t SQLiteDB::database_version(void) const
{
	const std::string q = "PRAGMA user_version";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 0)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	size_t version = 0;
	bool done = false;
	while (!done)
	{
		status = sqlite3_step(stmt);
		switch (status)
		{
			case SQLITE_ROW:
				version = static_cast<size_t>(sqlite3_column_int(stmt, 0));
				break;

			case SQLITE_DONE:
				done = true;
				break;

			case SQLITE_BUSY:
				// repeat query step
				break;

			default:
				throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
		}
	}

	sqlite3_finalize(stmt);
	return version;
}

void SQLiteDB::update_database_v1(void) const
{
	const std::string table_camera = "CREATE TABLE IF NOT EXISTS camera("
	                                 "id          INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
	                                 "position_x  REAL NOT NULL,"
	                                 "position_y  REAL NOT NULL,"
	                                 "position_z  REAL NOT NULL,"
	                                 "rotation_x  REAL NOT NULL,"
	                                 "rotation_y  REAL NOT NULL,"
	                                 "rotation_z  REAL NOT NULL,"
	                                 "rotation_w  REAL NOT NULL,"
	                                 "cursor_size REAL NOT NULL"
	                                 ")";

	run_command(table_camera);

	const std::string table_video = "CREATE TABLE IF NOT EXISTS video("
	                                "id       INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
	                                "hash     CHAR(64)        NOT NULL," // length of SHA256 hash *2 (bin->hex)
	                                "duration REAL            NOT NULL,"
	                                "name     TEXT            NOT NULL"
	                                " );";
	run_command(table_video);

	const std::string table_settings = "CREATE TABLE IF NOT EXISTS settings("
	                                   "video      INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
	                                   "projection INT  NOT NULL,"
	                                   "tiling     INT  NOT NULL,"
	                                   "stretch    BOOL NOT NULL,"
	                                   "angle      REAL NOT NULL,"
	                                   "zoom       REAL NOT NULL,"
	                                   "FOREIGN KEY(video) REFERENCES video(id)"
	                                   ")";
	run_command(table_settings);

	const std::string table_thumbnail = "CREATE TABLE IF NOT EXISTS thumbnail("
	                                    "video     INT  NOT NULL,"
	                                    "time      REAL NOT NULL,"
	                                    "format    INT  NOT NULL,"
	                                    "width     INT  NOT NULL,"
	                                    "height    INT  NOT NULL,"
	                                    "thumbnail BLOB NOT NULL,"
	                                    "FOREIGN KEY(video) REFERENCES video(id)"
	                                    ")";
	run_command(table_thumbnail);

	const std::string index_thumbnail = "CREATE UNIQUE INDEX IF NOT EXISTS idx_thumbnail ON thumbnail (video, time)";
	run_command(index_thumbnail);

	run_command("PRAGMA user_version = 1");
}

void SQLiteDB::update_database_v2(void) const
{
	const std::string add_slices = "alter table thumbnail add column slices int not null default 1";

	run_command(add_slices);
	run_command("PRAGMA user_version = 2");
}

void SQLiteDB::update_database_v3(void) const
{
	// find out all video IDs with thumbnails, ignore global preview strips.
	const std::string list_frames = "SELECT DISTINCT video FROM thumbnail WHERE time >= 0.0";
	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, list_frames.c_str(), static_cast<int>(list_frames.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + list_frames);
	}

	if (sqlite3_bind_parameter_count(stmt) != 0)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	std::set<size_t> video_ids;
	bool done = false;
	while (!done)
	{
		status = sqlite3_step(stmt);
		switch (status)
		{
			case SQLITE_ROW:
				video_ids.insert(static_cast<size_t>(sqlite3_column_int(stmt, 0)));
				break;

			case SQLITE_DONE:
				done = true;
				break;

			case SQLITE_BUSY:
				// repeat query step
				break;

			default:
				throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
		}
	}
	sqlite3_finalize(stmt);

	for (const size_t video : video_ids)
	{
		std::cout << "converting thumbnails for video " << video << std::endl;

		// load thumbnail timestamps, ignore global preview strips.
		const std::string q = "SELECT time FROM thumbnail WHERE video = ?1 AND time >= 0.0";
		status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("invalid SQL statement: " + q);
		}

		if (sqlite3_bind_parameter_count(stmt) != 1)
		{
			throw std::runtime_error("parameter count mismatch");
		}

		status = sqlite3_bind_int(stmt, 1, static_cast<int>(video));

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("failed binding parameter");
		}

		float time = 0.0f;
		std::vector<float> timestamps;
		done = false;
		while (!done)
		{
			status = sqlite3_step(stmt);
			switch (status)
			{
				case SQLITE_ROW:
					time = static_cast<float>(sqlite3_column_double(stmt, 0));
					timestamps.push_back(time);
					break;
				case SQLITE_DONE:
					done = true;
					break;
				case SQLITE_BUSY:
					// repeat query step
					break;

				default:
					throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
			}
		}
		sqlite3_finalize(stmt);

		SQLiteDB::format_t format = SQLiteDB::FORMAT_RGB24;
		size_t width = 0;
		size_t height = 0;
		size_t slices3d = 0;
		std::vector<uint8_t> pixels3d;
		for (const float timestamp : timestamps)
		{
			size_t slices = 0;
			std::vector<uint8_t> pixels;
			read_thumbnail(video, timestamp, format, width, height, slices, pixels);

			if ((format != SQLiteDB::FORMAT_RGB24) || (width == 0) || (height == 0) || (slices != 1) ||
			    (pixels.size() != width * height * slices * 3))
			{
				continue;
			}

			pixels3d.insert(pixels3d.end(), pixels.begin(), pixels.end());
			slices3d++;
		}

		if (slices3d == timestamps.size())
		{
			store_thumbnail(video, -2.0, format, width, height, slices3d, pixels3d);
		}

		// delete single frame thumbnails
		const std::string delete_thumbnails = "DELETE FROM thumbnail WHERE video=?1 AND time >= 0.0";
		status = sqlite3_prepare_v2(m_db, delete_thumbnails.c_str(), static_cast<int>(delete_thumbnails.size()), &stmt, NULL);

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("invalid SQL statement: " + delete_thumbnails);
		}

		if (sqlite3_bind_parameter_count(stmt) != 1)
		{
			throw std::runtime_error("parameter count mismatch");
		}

		status = sqlite3_bind_int(stmt, 1, static_cast<int>(video));

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("failed binding parameter");
		}

		done = false;
		while (!done)
		{
			status = sqlite3_step(stmt);
			switch (status)
			{
				case SQLITE_ROW:
					break;

				case SQLITE_DONE:
					done = true;
					break;

				case SQLITE_BUSY:
					// repeat query step
					break;

				default:
					throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
			}
		}
		sqlite3_finalize(stmt);
	}

	run_command("PRAGMA user_version = 3");
}

void SQLiteDB::update_database_v4(void) const
{
	const std::string add_switch = "alter table settings add column switch_eyes bool not null default false";
	const std::string add_mono = "alter table settings add column mono bool not null default false";

	run_command(add_switch);
	run_command(add_mono);
	run_command("PRAGMA user_version = 4");
}

void SQLiteDB::update_database(void) const
{
	const size_t version = database_version();

	std::vector<std::function<void(void)> > updates = {
		[this]() {
			update_database_v1();
		},
		[this]() {
			update_database_v2();
		},
		[this]() {
			update_database_v3();
		},
		[this]() {
			update_database_v4();
		}
	};

	if (version > updates.size())
	{
		throw std::runtime_error("Database version is newer than this application supports");
	}

	try
	{
		size_t i = 0;
		run_command("BEGIN EXCLUSIVE");

		while (i < updates.size())
		{
			if (version < i + 1)
			{
				updates[i]();
			}
			i++;
		}

		run_command("COMMIT");
	}
	catch (...)
	{
		sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, nullptr);
		throw;
	}
}

size_t SQLiteDB::get_video_id_from_hash(const std::string& hash) const
{
	const std::string q = "SELECT id FROM video WHERE hash = ?1";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 1)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	status = sqlite3_bind_text(stmt, 1, hash.c_str(), static_cast<int>(hash.size()), SQLITE_STATIC);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	bool done = false;
	size_t video_id = 0;
	while (!done)
	{
		status = sqlite3_step(stmt);
		switch (status)
		{
			case SQLITE_ROW:
				video_id = static_cast<size_t>(sqlite3_column_int(stmt, 0));
				break;

			case SQLITE_DONE:
				done = true;
				break;

			case SQLITE_BUSY:
				// repeat query step
				break;

			default:
				throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
		}
	}

	sqlite3_finalize(stmt);
	return video_id;
}

size_t SQLiteDB::get_video_id_from_name(const std::string& filename) const
{
	const std::string q = "SELECT id FROM video WHERE name = ?1";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 1)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	status = sqlite3_bind_text(stmt, 1, filename.c_str(), static_cast<int>(filename.size()), SQLITE_STATIC);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	bool done = false;
	size_t video_id = 0;
	while (!done)
	{
		status = sqlite3_step(stmt);
		switch (status)
		{
			case SQLITE_ROW:
				video_id = static_cast<size_t>(sqlite3_column_int(stmt, 0));
				break;

			case SQLITE_DONE:
				done = true;
				break;

			case SQLITE_BUSY:
				// repeat query step
				break;

			default:
				throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
		}
	}

	sqlite3_finalize(stmt);
	return video_id;
}

size_t SQLiteDB::get_video_id(const std::string& filename) const
{
	size_t video_id = get_video_id_from_name(filename);

	if (video_id == 0)
	{
		const std::string hash = checksum_file(filename);
		video_id = get_video_id_from_hash(hash);
	}
	return video_id;
}

size_t SQLiteDB::read_video(const std::string& filename, float& duration) const
{
	size_t video_id = get_video_id(filename);

	if (video_id == 0)
	{
		duration = 0.0;
		return 0;
	}

	const std::string q = "SELECT duration FROM video WHERE id = ?1";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 1)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	status = sqlite3_bind_int(stmt, 1, static_cast<int>(video_id));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	bool done = false;
	while (!done)
	{
		status = sqlite3_step(stmt);
		switch (status)
		{
			case SQLITE_ROW:
				duration = static_cast<float>(sqlite3_column_double(stmt, 0));
				break;

			case SQLITE_DONE:
				done = true;
				break;

			case SQLITE_BUSY:
				// repeat query step
				break;

			default:
				throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
		}
	}

	sqlite3_finalize(stmt);
	return video_id;
}

size_t SQLiteDB::store_video(const std::string& file_name, const float duration) const
{
	size_t video_id = get_video_id_from_name(file_name);

	std::string hash;

	if (video_id == 0)
	{
		hash = checksum_file(file_name);
		video_id = get_video_id_from_hash(hash);
	}

	if (video_id == 0)
	{
		/* video not yet known */
		const std::string q = "INSERT INTO video(hash,duration,name) values(?1, ?2, ?3)";

		sqlite3_stmt* stmt;
		int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("invalid SQL statement: " + q);
		}

		if (sqlite3_bind_parameter_count(stmt) != 3)
		{
			throw std::runtime_error("parameter count mismatch");
		}

		status = sqlite3_bind_text(stmt, 1, hash.c_str(), static_cast<int>(hash.size()), SQLITE_STATIC);

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("failed binding parameter");
		}

		status = sqlite3_bind_double(stmt, 2, static_cast<double>(duration));

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("failed binding parameter");
		}

		status = sqlite3_bind_text(stmt, 3, file_name.c_str(), static_cast<int>(file_name.size()), SQLITE_STATIC);

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("failed binding parameter");
		}

		do
		{
			status = sqlite3_step(stmt);
		}
		while (status == SQLITE_BUSY);

		if (status != SQLITE_DONE)
		{
			throw std::runtime_error("failed inserting video: " + std::to_string(status));
		}
		sqlite3_finalize(stmt);

		video_id = get_video_id_from_hash(hash);
	}
	else
	{
		const std::string q = "UPDATE video SET duration=?2,name=?3 WHERE id=?1";

		sqlite3_stmt* stmt;
		int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("invalid SQL statement: " + q);
		}

		if (sqlite3_bind_parameter_count(stmt) != 3)
		{
			throw std::runtime_error("parameter count mismatch");
		}

		status = sqlite3_bind_int(stmt, 1, static_cast<int>(video_id));

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("failed binding parameter");
		}

		status = sqlite3_bind_double(stmt, 2, static_cast<double>(duration));

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("failed binding parameter");
		}

		status = sqlite3_bind_text(stmt, 3, file_name.c_str(), static_cast<int>(file_name.size()), SQLITE_STATIC);

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("failed binding parameter");
		}

		do
		{
			status = sqlite3_step(stmt);
		}
		while (status == SQLITE_BUSY);

		if (status != SQLITE_DONE)
		{
			throw std::runtime_error("video update failed.");
		}

		sqlite3_finalize(stmt);
	}

	return video_id;
}

size_t SQLiteDB::store_window(const std::string& name) const
{
	size_t id = get_video_id_from_name(name);

	if (id == 0)
	{
		/* video not yet known */
		const std::string q = "INSERT INTO video(hash,duration,name) values("", 0.0, ?1)";

		sqlite3_stmt* stmt;
		int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("invalid SQL statement: " + q);
		}

		if (sqlite3_bind_parameter_count(stmt) != 1)
		{
			throw std::runtime_error("parameter count mismatch");
		}

		status = sqlite3_bind_text(stmt, 1, name.c_str(), static_cast<int>(name.size()), SQLITE_STATIC);

		if (status != SQLITE_OK)
		{
			throw std::runtime_error("failed binding parameter");
		}

		do
		{
			status = sqlite3_step(stmt);
		}
		while (status == SQLITE_BUSY);

		if (status != SQLITE_DONE)
		{
			throw std::runtime_error("failed inserting video: " + std::to_string(status));
		}
		sqlite3_finalize(stmt);

		id = get_video_id_from_name(name);
	}

	return id;
}

void SQLiteDB::read_settings(const size_t video_id,
                             size_t& projection,
                             size_t& tiling,
                             bool& stretch,
                             float& angle,
                             float& zoom,
                             bool& switch_eyes,
                             bool& mono) const
{
	projection = 0;
	tiling = 0;
	stretch = false;
	angle = 0.0f;
	zoom = 0.0f;
	switch_eyes = false;
	mono = false;

	const std::string q = "SELECT projection,tiling,stretch,angle,zoom,switch_eyes,mono FROM settings WHERE video = ?1";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 1)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	status = sqlite3_bind_int(stmt, 1, static_cast<int>(video_id));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	bool done = false;
	while (!done)
	{
		status = sqlite3_step(stmt);
		switch (status)
		{
			case SQLITE_ROW:
				projection  = static_cast<size_t>(sqlite3_column_int(stmt, 0));
				tiling      = static_cast<size_t>(sqlite3_column_int(stmt, 1));
				stretch     = static_cast<bool>(sqlite3_column_int(stmt, 2));
				angle       = static_cast<float>(sqlite3_column_double(stmt, 3));
				zoom        = static_cast<float>(sqlite3_column_double(stmt, 4));
				switch_eyes = static_cast<bool>(sqlite3_column_int(stmt, 5));
				mono        = static_cast<bool>(sqlite3_column_int(stmt, 6));
				break;
			case SQLITE_DONE:
				done = true;
				break;
			case SQLITE_BUSY:
				// repeat query step
				break;

			default:
				throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
		}
	}

	sqlite3_finalize(stmt);
}

void SQLiteDB::store_settings(const size_t video_id,
                              const size_t projection,
                              const size_t tiling,
                              const bool stretch,
                              const float angle,
                              const float zoom,
                              const bool switch_eyes,
                              const bool mono) const
{
	const std::string q = "INSERT OR REPLACE INTO "
	                      "settings (video, projection, tiling, stretch, angle, zoom, switch_eyes, mono) "
	                      "values(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 8)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	status = sqlite3_bind_int(stmt, 1, static_cast<int>(video_id));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_int(stmt, 2, static_cast<int>(projection));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_int(stmt, 3, static_cast<int>(tiling));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_int(stmt, 4, stretch);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 5, static_cast<double>(angle));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 6, static_cast<double>(zoom));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_int(stmt, 7, switch_eyes);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_int(stmt, 8, mono);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	do
	{
		status = sqlite3_step(stmt);
	}
	while (status == SQLITE_BUSY);

	if (status != SQLITE_DONE)
	{
		throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
	}

	sqlite3_finalize(stmt);
}

void SQLiteDB::read_thumbnail(const size_t video_id,
                              const float time,
                              format_t& format,
                              size_t& width,
                              size_t& height,
                              size_t& slices,
                              std::vector<uint8_t>& data) const
{
	const std::string q = "SELECT format,width,height,slices,thumbnail FROM thumbnail WHERE video = ?1 AND ABS(time - $2) < 0.1";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 2)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	status = sqlite3_bind_int(stmt, 1, static_cast<int>(video_id));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 2, static_cast<double>(time));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	format = FORMAT_RGB24;
	width = 0;
	height = 0;
	slices = 0;
	data.clear();
	const uint8_t* ptr;
	size_t bytes = 0;
	bool done = false;
	while (!done)
	{
		status = sqlite3_step(stmt);
		switch (status)
		{
			case SQLITE_ROW:
				format = static_cast<format_t>(sqlite3_column_int(stmt, 0));
				width  = static_cast<size_t>(sqlite3_column_int(stmt, 1));
				height = static_cast<size_t>(sqlite3_column_int(stmt, 2));
				slices = static_cast<size_t>(sqlite3_column_int(stmt, 3));
				bytes  = static_cast<size_t>(sqlite3_column_bytes(stmt, 4));
				ptr    = static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 4));
				data.assign(ptr, ptr + bytes);
				break;
			case SQLITE_DONE:
				done = true;
				break;
			case SQLITE_BUSY:
				// repeat query step
				break;

			default:
				throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
		}
	}

	sqlite3_finalize(stmt);
}

void SQLiteDB::store_thumbnail(const size_t video_id,
                               const float time,
                               const format_t format,
                               const size_t width,
                               const size_t height,
                               const size_t slices,
                               const std::vector<uint8_t>& data) const
{
	const std::string q = "INSERT OR REPLACE INTO thumbnail (video, time, format, width, height, slices, thumbnail) values (?1, ?2, ?3, ?4, ?5, ?6, ?7)";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 7)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	status = sqlite3_bind_int(stmt, 1, static_cast<int>(video_id));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 2, static_cast<double>(time));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_int(stmt, 3, static_cast<int>(format));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_int(stmt, 4, static_cast<int>(width));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_int(stmt, 5, static_cast<int>(height));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_int(stmt, 6, static_cast<int>(slices));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	if (data.size() > 1000000000)
	{
		throw std::runtime_error("thumbnails too big.");
	}

	status = sqlite3_bind_blob(stmt, 7, data.data(), static_cast<int>(data.size()), SQLITE_STATIC);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	do
	{
		status = sqlite3_step(stmt);
	}
	while (status == SQLITE_BUSY);

	if (status != SQLITE_DONE)
	{
		throw std::runtime_error("invalid query result (line " + std::to_string(__LINE__) + "): " + std::to_string(status));
	}

	sqlite3_finalize(stmt);
}

void SQLiteDB::read_camera(glm::vec3& position,
                           glm::quat& rotation,
                           float& cursor_size) const
{
	position = glm::vec3(0.0, 0.0, 0.0);
	rotation = glm::quat(1.0, 0.0, 0.0, 1.0);
	cursor_size = 0.1f;

	const std::string q = "SELECT position_x,position_y,position_z,"
	                      "rotation_x,rotation_y,rotation_z,rotation_w,"
	                      "cursor_size"
	                      " FROM camera";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 0)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	position = glm::vec3(0.0, 0.0, 0.0);
	rotation = glm::quat(0.0, 0.0, 0.0, 0.0);

	bool done = false;
	while (!done)
	{
		status = sqlite3_step(stmt);
		switch (status)
		{
			case SQLITE_ROW:
				position.x  = static_cast<float>(sqlite3_column_double(stmt, 0));
				position.y  = static_cast<float>(sqlite3_column_double(stmt, 1));
				position.z  = static_cast<float>(sqlite3_column_double(stmt, 2));
				rotation.x  = static_cast<float>(sqlite3_column_double(stmt, 3));
				rotation.y  = static_cast<float>(sqlite3_column_double(stmt, 4));
				rotation.z  = static_cast<float>(sqlite3_column_double(stmt, 5));
				rotation.w  = static_cast<float>(sqlite3_column_double(stmt, 6));
				cursor_size = static_cast<float>(sqlite3_column_double(stmt, 7));
				break;
			case SQLITE_DONE:
				done = true;
				break;
			case SQLITE_BUSY:
				// repeat query step
				break;

			default:
				throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
		}
	}

	sqlite3_finalize(stmt);
}

void SQLiteDB::store_camera(const glm::vec3& position,
                            const glm::quat& rotation,
                            const float& cursor_size) const
{
	const std::string q = "INSERT OR REPLACE INTO camera (id,position_x,position_y,position_z,"
	                      "rotation_x,rotation_y,rotation_z,rotation_w,"
	                      "cursor_size) "
	                      "values (1, ?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)";

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(m_db, q.c_str(), static_cast<int>(q.size()), &stmt, NULL);

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("invalid SQL statement: " + q);
	}

	if (sqlite3_bind_parameter_count(stmt) != 8)
	{
		throw std::runtime_error("parameter count mismatch");
	}

	status = sqlite3_bind_double(stmt, 1, static_cast<double>(position.x));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 2, static_cast<double>(position.y));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 3, static_cast<double>(position.z));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 4, static_cast<double>(rotation.x));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 5, static_cast<double>(rotation.y));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 6, static_cast<double>(rotation.z));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 7, static_cast<double>(rotation.w));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	status = sqlite3_bind_double(stmt, 8, static_cast<double>(cursor_size));

	if (status != SQLITE_OK)
	{
		throw std::runtime_error("failed binding parameter");
	}

	do
	{
		status = sqlite3_step(stmt);
	}
	while (status == SQLITE_BUSY);

	if (status != SQLITE_DONE)
	{
		throw std::runtime_error("invalid query result (" + std::to_string(__LINE__) + "): " + std::to_string(status));
	}

	sqlite3_finalize(stmt);
}
