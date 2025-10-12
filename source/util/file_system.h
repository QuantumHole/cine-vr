// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <vector>

class FileSystem
{
	private:
		std::vector<std::string> select_files(const std::string& dir, const bool use_files, const bool use_dirs) const;

	public:
		FileSystem(void);
		std::string current_directory(void) const;
		std::vector<std::string> split_path(const std::string& path) const;
		std::vector<std::string> file_names(const std::string& dir) const;
		std::vector<std::string> directory_names(const std::string& dir) const;
};

#endif
