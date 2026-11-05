// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnail_store.h"

#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <stdexcept>
#include <string>

static std::mutex thumbnail_jobs_mutex;
static std::set<std::string> thumbnail_jobs;

static void generate_thumbnails(const std::string& video_file, size_t video_id)
{
	try
	{
		ThumbnailGenerator generator;
		const std::vector<Thumbnail> thumbnails = generator.generate(video_file);

		std::cout << "received " << thumbnails.size() << " thumbnails for " << video_file << std::endl;
		SQLiteDB database("cine-vr");

		if (video_id == 0)
		{
			// video_id = database.store_video(video_file, 0.0f);
		}

		// bool stored_global_thumbnail = false;
		// for (const Thumbnail& thumbnail : thumbnails)
		// {
		// const float timestamp = static_cast<float>(thumbnail.timestampMs) / 1000.0f;
		// database.store_thumbnail(video_id, timestamp, GL_RGBA,
		// static_cast<size_t>(thumbnail.width),
		// static_cast<size_t>(thumbnail.height), thumbnail.pixels);
		// if (!stored_global_thumbnail)
		// {
		// database.store_thumbnail(video_id, -1.0f, GL_RGBA,
		// static_cast<size_t>(thumbnail.width),
		// static_cast<size_t>(thumbnail.height), thumbnail.pixels);
		// stored_global_thumbnail = true;
		// }
		// }
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
	m_file_path(file_path),
	m_database("cine-vr")
{
}

Thumbnail ThumbnailStore::get_thumbnail(const float timestamp) const
{
	Thumbnail thumbnail;

	try
	{
		float duration = 0.0f;
		size_t format = 0;
		size_t width = 0;
		size_t height = 0;
		std::vector<uint8_t> data;
		const size_t video_id = m_database.read_video(m_file_path, duration);
		std::cout << m_file_path << ", video_id: " << video_id << ", duration: " << duration << std::endl;

		if (video_id)
		{
			std::cout << m_file_path << ": reading thumbnail for video_id: " << video_id << std::endl;
			m_database.read_thumbnail(video_id, timestamp, format, width, height, data);
		}

		if (data.empty())
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
