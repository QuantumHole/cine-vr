// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <string>
#include <sqlite3.h>
#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>
#include <GL/glew.h>

/** SQLite database interface for storing video metadata, thumbnails, and configuration. */
class SQLiteDB
{
	private:
		sqlite3* m_db;          ///< SQLite database connection

		/** Copy constructor (deleted) */
		SQLiteDB(const SQLiteDB&);

		/** Assignment operator (deleted) */
		SQLiteDB& operator=(const SQLiteDB&);

		/** Run an SQL command.
		 * @param command SQL command to execute.
		 */
		void run_command(const std::string& command) const;

		/** Get video ID from hash.
		 * @param hash Video hash.
		 * @return Video ID.
		 */
		size_t get_video_id_from_hash(const std::string& hash) const;

		/** Get video ID from filename.
		 * @param filename Video filename.
		 * @return Video ID.
		 */
		size_t get_video_id_from_name(const std::string& filename) const;

		/** Get video ID, either from file name, or from its hash, if file name is unknown.
		 * @param filename Video filename.
		 * @return Video ID.
		 */
		size_t get_video_id(const std::string& filename) const;

		/** calculate checksum of given file.
		 * The checksum is calculated from a 1MiB chunk in the middle of the file.
		 * If the file is smaller than 1MiB, the checksum is calculated from the full file.
		 * @param path source file
		 * @return hash value
		 */
		std::string checksum_file(const std::string& path) const;

	public:
		/** Constructor.
		 * @param name Database name.
		 */
		explicit SQLiteDB(const std::string& name);

		/** Destructor. */
		~SQLiteDB(void);

		/** Read camera parameters.
		 * @param position Camera position.
		 * @param rotation Camera rotation.
		 * @param cursor_size Cursor size.
		 */
		void read_camera(glm::vec3& position,
		                 glm::quat& rotation,
		                 float& cursor_size) const;

		/** Store camera parameters.
		 * @param position Camera position.
		 * @param rotation Camera rotation.
		 * @param cursor_size Cursor size.
		 */
		void store_camera(const glm::vec3& position,
		                  const glm::quat& rotation,
		                  const float& cursor_size) const;

		/** Read video information.
		 * @param name Video name.
		 * @param duration Video duration.
		 * @return Video ID.
		 */
		size_t read_video(const std::string& name,
		                  float& duration) const;

		/** Store video information.
		 * @param file_name Video file name.
		 * @param duration Video duration.
		 * @return Video ID.
		 */
		size_t store_video(const std::string& file_name,
		                   const float duration) const;

		/** Store window information.
		 * @param name Window name.
		 * @return Window ID.
		 */
		size_t store_window(const std::string& name) const;

		/** Read video settings.
		 * @param video_id Video ID.
		 * @param projection Projection type.
		 * @param tiling Tiling type.
		 * @param stretch Stretch mode.
		 * @param angle Angle.
		 * @param zoom Zoom level.
		 */
		void read_settings(const size_t video_id,
		                   size_t& projection,
		                   size_t& tiling,
		                   bool& stretch,
		                   float& angle,
		                   float& zoom) const;

		/** Store video settings.
		 * @param video_id Video ID.
		 * @param projection Projection type.
		 * @param tiling Tiling type.
		 * @param stretch Stretch mode.
		 * @param angle Angle.
		 * @param zoom Zoom level.
		 */
		void store_settings(const size_t video_id,
		                    const size_t projection,
		                    const size_t tiling,
		                    const bool stretch,
		                    const float angle,
		                    const float zoom) const;

		/** Read thumbnail information.
		 * @param video_id Video ID.
		 * @param time Timestamp of the thumbnail.
		 * @param format Thumbnail format.
		 * @param width Thumbnail width.
		 * @param height Thumbnail height.
		 * @param data Thumbnail data.
		 */
		void read_thumbnail(const size_t video_id,
		                    const float time,
		                    size_t& format,
		                    size_t& width,
		                    size_t& height,
		                    std::vector<uint8_t>& data) const;

		/** Store thumbnail information.
		 * @param video_id Video ID.
		 * @param time Timestamp of the thumbnail.
		 * @param format Thumbnail format.
		 * @param width Thumbnail width.
		 * @param height Thumbnail height.
		 * @param data Thumbnail data.
		 */
		void store_thumbnail(const size_t video_id,
		                     const float time,
		                     const size_t format,
		                     const size_t width,
		                     const size_t height,
		                     const std::vector<uint8_t>& data) const;

		/** Read all thumbnail timestamps for a given video.
		 * @param video_id Video ID.
		 * @return Vector of thumbnail timestamps.
		 */
		std::vector<float> read_thumb_timestamps(const size_t video_id) const;
};

#endif
