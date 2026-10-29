// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "file_system.h"

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdexcept>
#include <linux/limits.h>
#include <algorithm>
#include <fstream>

/** separator between path sections. */
static const std::string directory_separator = "/";

/** default constructor. */
FileSystem::FileSystem(void)
{
}

/** extracts the file extension from a file name.
 * @param name file name.
 * @return file extension.
 */
std::string FileSystem::extension(const std::string& name) const
{
	const size_t index = name.rfind(".");

	if (index == std::string::npos)
	{
		return "";
	}

	std::string ext = name.substr(index + 1);

	/* convert to lower case characters */
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){
		return std::tolower(c);
	});

	return ext;
}

/** checks if the file extesion represents an image.
 * @param ext file extension.
 * @return true if the extension represents an image.
 */
bool FileSystem::is_image(const std::string& ext) const
{
	static const std::set<std::string> extensions = {
		"png",
		"jpg",
		"jpeg",
		"tga",
		"bmp"
	};

	return extensions.find(ext) != extensions.end();
}

/** checks if the file extesion represents a video.
 * @param ext file extension.
 * @return true if the extension represents a video.
 */
bool FileSystem::is_video(const std::string& ext) const
{
	static const std::set<std::string> extensions = {
		"avi",
		"mkv",
		"mpg",
		"mp4"
	};

	return extensions.find(ext) != extensions.end();
}

/** checks if the file extesion represents a vector graphics.
 * @param ext file extension.
 * @return true if the extension represents a vector graphics.
 */
bool FileSystem::is_vector(const std::string& ext) const
{
	static const std::set<std::string> extensions = {
		"svg"
	};

	return extensions.find(ext) != extensions.end();
}

/** split path into directory/file components.
 * @param path input path.
 * @return vector of path elements split at directory separator.
 */
std::vector<std::string> FileSystem::split_path(const std::string& path) const
{
	std::vector<std::string> parts;

	size_t last = 0;

	for (size_t i = path.find(directory_separator); i != std::string::npos; i = path.find(directory_separator, last))
	{
		const std::string s = path.substr(last, i - last);

		parts.push_back(s);
		last = i + 1;
	}

	const std::string s = path.substr(last);
	parts.push_back(s);

	return parts;
}

/** join path elements with directory separator into a string.
 * @param start start iterator for elements of a vector.
 * @param end end iterator for elements of a vector.
 * @return vector elements joint with directory separators.
 */
std::string FileSystem::join_path(const std::vector<std::string>::const_iterator start, const std::vector<std::string>::const_iterator end) const
{
	std::string path;

	for (std::vector<std::string>::const_iterator iter = start; iter != end; ++iter)
	{
		const size_t index = path.rfind(directory_separator);

		if ((index == std::string::npos) || (index < path.size() - directory_separator.size()))
		{
			path += directory_separator;
		}
		path += *iter;
	}
	return path;
}

bool FileSystem::is_file(const std::string& name) const
{
	struct stat sb;

	return !stat(name.c_str(), &sb) && S_ISREG(sb.st_mode);
}

bool FileSystem::is_directory(const std::string& name) const
{
	struct stat sb;

	return !stat(name.c_str(), &sb) && S_ISDIR(sb.st_mode);
}

std::set<std::string> FileSystem::select_files(const std::string& dir, const bool use_files, const bool use_dirs, const bool show_hidden) const
{
	std::set<std::string> entries;
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
		const std::string ext = extension(en);

		if ((en != ".") &&
		    (en != "..") &&
		    (!en.empty()) &&
		    (show_hidden || (!show_hidden && (en[0] != '.'))) &&
		    ((use_files && is_file(name) && (is_image(ext) || is_video(ext))) ||
		     (use_dirs && is_directory(name))))
		{
			entries.insert(en);
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

std::set<std::string> FileSystem::file_names(const std::string& dir, const bool show_hidden) const
{
	return select_files(dir, true, false, show_hidden);
}

std::set<std::string> FileSystem::directory_names(const std::string& dir, const bool show_hidden) const
{
	return select_files(dir, false, true, show_hidden);
}

std::string FileSystem::read_file(const std::string& path) const
{
	std::ifstream sourceFile(path);

	// Source file loaded
	if (!sourceFile.is_open())
	{
		throw std::runtime_error("Unable to open file " + path);
	}

	std::string content;
	content.assign((std::istreambuf_iterator< char >(sourceFile)), std::istreambuf_iterator< char >());

	return content;
}
