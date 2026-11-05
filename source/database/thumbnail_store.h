// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef THUMBNAIL_STORE_H
#define THUMBNAIL_STORE_H

#include "sqlitedb.h"
#include "thumbnail_generator.h"

#include <string>

class ThumbnailStore
{
	private:
		const std::string m_file_path;   ///< path to the current video file
		SQLiteDB m_database;             ///< SQLite database instance

	public:
		explicit ThumbnailStore(const std::string& file_path);

		Thumbnail get_thumbnail(const float timestamp) const;
};

#endif
