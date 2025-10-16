// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <vector>
#include <set>

class FileSystem
{
	private:
		std::set<std::string> select_files(const std::string& dir, const bool use_files, const bool use_dirs, const bool show_hidden) const;
		std::string extension(const std::string& name) const;
		bool is_image(const std::string& ext) const;
		bool is_video(const std::string& ext) const;

	public:
		FileSystem(void);
		std::string current_directory(void) const;
		std::vector<std::string> split_path(const std::string& path) const;
		std::string join_path(const std::vector<std::string>::const_iterator start, const std::vector<std::string>::const_iterator end) const;
		std::set<std::string> file_names(const std::string& dir, const bool show_hidden = false) const;
		std::set<std::string> directory_names(const std::string& dir, const bool show_hidden = false) const;
};

#endif
