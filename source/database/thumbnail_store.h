// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef THUMBNAIL_STORE_H
#define THUMBNAIL_STORE_H

#include "thumbnail_generator.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct ThumbnailVolume
{
	ThumbnailVolume(void) :
		size(0, 0, 0),
		duration(0.0),
		pixels()
	{
	}
	glm::uvec3 size;
	float duration;
	std::vector<uint8_t> pixels;
};

class ThumbnailStore
{
	private:
		const std::string m_file_path;   ///< path to the current video file

	public:
		explicit ThumbnailStore(const std::string& file_path);

		Thumbnail get_thumbnail(const float timestamp) const;
		ThumbnailVolume get_thumbnail_volume(void) const;
};

#endif
