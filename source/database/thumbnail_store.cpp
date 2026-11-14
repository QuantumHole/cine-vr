// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnail_store.h"
#include "application.h"

#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <stdexcept>
#include <string>
#include <limits>
#include <algorithm>

static std::mutex thumbnail_jobs_mutex;
static std::set<std::string> thumbnail_jobs;

static Thumbnail mix_frames(const std::vector<Thumbnail>& thumbnails)
{
	Thumbnail timeline;

	if (thumbnails.empty())
	{
		return timeline;
	}

	if ((thumbnails.size() > static_cast<size_t>(std::numeric_limits<int>::max() / 3)) ||
	    (thumbnails[0].height <= 0))
	{
		throw std::runtime_error("Invalid thumbnail dimensions for timeline");
	}

	timeline.width = static_cast<int>(thumbnails.size());
	timeline.height = thumbnails[0].height;
	timeline.stride = timeline.width * 3;
	timeline.timestampMs = -1;
	timeline.pixels.resize(static_cast<size_t>(timeline.stride) * static_cast<size_t>(timeline.height));

	const size_t tn_stride = static_cast<size_t>(thumbnails[0].stride);
	for (size_t frameIndex = 0; frameIndex < thumbnails.size(); frameIndex++)
	{
		const Thumbnail& thumbnail = thumbnails[frameIndex];

		if ((thumbnail.width <= 0) || (thumbnail.height <= 0) ||
		    (static_cast<size_t>(thumbnail.stride) != tn_stride) ||
		    (thumbnail.pixels.size() != static_cast<size_t>(thumbnail.stride) * static_cast<size_t>(thumbnail.height)))
		{
			throw std::runtime_error("Invalid thumbnail buffer while building timeline");
		}

		for (size_t row = 0; row < static_cast<size_t>(timeline.height); row++)
		{
			const size_t sourceRowOffset = row * static_cast<size_t>(thumbnail.stride);
			const size_t targetOffset = row * static_cast<size_t>(timeline.stride) + frameIndex * 3;

			for (size_t channel = 0; channel < 3; channel++)
			{
				uint64_t sum = 0;
				for (int column = 0; column < thumbnail.width; column++)
				{
					const size_t sourceOffset = sourceRowOffset + static_cast<size_t>(column) * 3 + channel;
					sum += thumbnail.pixels[sourceOffset];
				}

				timeline.pixels[targetOffset + channel] = static_cast<uint8_t>((sum + static_cast<uint64_t>(thumbnail.width / 2)) /
				                                                               static_cast<uint64_t>(thumbnail.width));
			}
		}
	}

	return timeline;
}

static void generate_thumbnails(const std::string& video_file, size_t video_id)
{
	try
	{
		ThumbnailGenerator generator(video_file);
		const std::vector<Thumbnail> thumbnails = generator.generate();

		std::cout << "received " << thumbnails.size() << " thumbnails for " << video_file << std::endl;
		const SQLiteDB& database = Application::instance().database();

		if (video_id == 0)
		{
			video_id = database.store_video(video_file, static_cast<float>(generator.duration()));
		}

		const SQLiteDB::format_t tn_format = SQLiteDB::FORMAT_RGB24;    // uncompressed 24bit RGB values
		Thumbnail thumbs;
		for (const Thumbnail& thumbnail : thumbnails)
		{
			thumbs.width = thumbnail.width;
			thumbs.height = thumbnail.height;
			thumbs.pixels.insert(thumbs.pixels.end(), thumbnail.pixels.begin(), thumbnail.pixels.end());
		}
		database.store_thumbnail(
			video_id, -2.0f, tn_format,
			static_cast<size_t>(thumbs.width),
			static_cast<size_t>(thumbs.height),
			thumbnails.size(),
			thumbs.pixels);

		const Thumbnail timeline = mix_frames(thumbnails);
		database.store_thumbnail(
			video_id, -1.0f, tn_format,
			static_cast<size_t>(timeline.width),
			static_cast<size_t>(timeline.height),
			1,
			timeline.pixels);
	}
	catch (const std::exception& error)
	{
		std::cerr << "Failed to generate thumbnails for " << video_file << ": " << error.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Failed to generate thumbnails for " << video_file << std::endl;
	}

	std::lock_guard<std::mutex> lock(thumbnail_jobs_mutex);
	thumbnail_jobs.erase(video_file);
}

ThumbnailStore::ThumbnailStore(const std::string& file_path) :
	m_file_path(file_path)
{
}

Thumbnail ThumbnailStore::get_thumbnail(const float timestamp) const
{
	Thumbnail thumbnail;

	try
	{
		const SQLiteDB& database = Application::instance().database();

		float duration = 0.0f;
		SQLiteDB::format_t format = SQLiteDB::FORMAT_RGB24;
		size_t width = 0;
		size_t height = 0;
		size_t slices = 0;
		std::vector<uint8_t> data;
		const size_t video_id = database.read_video(m_file_path, duration);

		if (video_id)
		{
			database.read_thumbnail(video_id, timestamp, format, width, height, slices, data);
		}

		if (!data.empty() && (format == SQLiteDB::FORMAT_RGB24) && (width > 0) && (height > 0) &&
		    (width <= static_cast<size_t>(std::numeric_limits<int>::max() / 3)) &&
		    (height <= static_cast<size_t>(std::numeric_limits<int>::max())))
		{
			const size_t stride = width * 3;

			if (data.size() >= stride * height)
			{
				thumbnail.width = static_cast<int>(width);
				thumbnail.height = static_cast<int>(height);
				thumbnail.stride = static_cast<int>(stride);
				thumbnail.timestampMs = -1;
				thumbnail.pixels = std::move(data);
			}
		}

		if (thumbnail.pixels.empty())
		{
			std::cout << m_file_path << ": No thumbnail found for video_id: " << video_id << ", generating..." << std::endl;
			std::lock_guard<std::mutex> lock(thumbnail_jobs_mutex);

			if (thumbnail_jobs.insert(m_file_path).second)
			{
				try
				{
					std::thread(generate_thumbnails, m_file_path, video_id).detach();
				}
				catch (...)
				{
					thumbnail_jobs.erase(m_file_path);
					throw;
				}
			}
		}
	}
	catch (const std::exception& error)
	{
		std::cerr << "Failed to load thumbnail for " << m_file_path << ": " << error.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Failed to load thumbnail for " << m_file_path << std::endl;
	}

	return thumbnail;
}

ThumbnailVolume ThumbnailStore::get_thumbnail_volume(void) const
{
	ThumbnailVolume volume;
	float duration = 0.0f;
	const SQLiteDB& database = Application::instance().database();
	const size_t video_id = database.read_video(m_file_path, duration);

	if (!video_id)
	{
		return volume;
	}

	SQLiteDB::format_t format = SQLiteDB::FORMAT_RGB24;
	size_t width = 0;
	size_t height = 0;
	size_t slices = 0;
	std::vector<uint8_t> pixels;
	database.read_thumbnail(video_id, -2.0, format, width, height, slices, pixels);

	if ((format != SQLiteDB::FORMAT_RGB24) || (width == 0) || (height == 0) || (slices == 0) ||
	    (pixels.size() != width * height * slices * 3))
	{
		return volume;
	}

	volume.duration = duration;
	volume.size = glm::uvec3(
		static_cast<unsigned int>(width),
		static_cast<unsigned int>(height),
		static_cast<unsigned int>(slices));
	volume.pixels = pixels;

	return volume;
}
