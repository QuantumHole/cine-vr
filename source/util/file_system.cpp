// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "file_system.h"

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdexcept>
#include <linux/limits.h>
#include <iostream>

static const std::string directory_separator = "/";

FileSystem::FileSystem(void)
{
}

std::vector<std::string> FileSystem::split_path(const std::string& path) const
{
	std::vector<std::string> parts;

	size_t last = 0;

	for (size_t i = path.find(directory_separator); i != std::string::npos; i = path.find(directory_separator, last))
	{
		const std::string s = path.substr(last, i - last);
		std::cout << "directory part: " << s << std::endl;

		if (s.empty())
		{
			parts.push_back(directory_separator);
		}
		else
		{
			parts.push_back(s);
		}
		last = i + 1;
	}

	const std::string s = path.substr(last);
	std::cout << "directory part: " << s << std::endl;
	parts.push_back(s);

	return parts;
}

std::vector<std::string> FileSystem::select_files(const std::string& dir, const bool use_files, const bool use_dirs) const
{
	std::vector<std::string> entries;
	DIR* direct = opendir(dir.c_str());

	if (direct == NULL)
	{
		throw std::runtime_error("could not open directory " + dir);
	}

	/* loop over all the files and directories within directory */
	for (struct dirent* ent = readdir(direct); ent; ent = readdir(direct))
	{
		const std::string en = ent->d_name;
		const std::string name = dir + directory_separator + en;

		struct stat sb;

		if (!stat(name.c_str(), &sb) &&
		    (en != ".") &&
		    (en != "..") &&
		    ((use_files && S_ISREG(sb.st_mode)) ||
		     (use_dirs && S_ISDIR(sb.st_mode))))
		{
			entries.push_back(en);
		}
	}
	closedir(direct);
	return entries;
}

std::string FileSystem::current_directory(void) const
{
	std::vector<char> current_path(PATH_MAX, 0);

	const char* dir = getcwd(current_path.data(), current_path.size());

	if (dir != current_path.data())
	{
		return "";
	}
	return std::string(dir);
}

std::vector<std::string> FileSystem::file_names(const std::string& dir) const
{
	return select_files(dir, true, false);
}

std::vector<std::string> FileSystem::directory_names(const std::string& dir) const
{
	return select_files(dir, false, true);
}
