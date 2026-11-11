// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnail_generator.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <iostream>

static std::string ffmpegError(int error)
{
	char buffer[AV_ERROR_MAX_STRING_SIZE];

	av_strerror(error, buffer, sizeof(buffer));
	return buffer;
}

ThumbnailGenerator::ThumbnailGenerator(const std::string& file_path) :
	m_thumbnail_height(128),
	m_file_path(file_path),
	m_duration(0),
	m_stepsize(0),
	m_thumbnails(),
	m_sws(nullptr),
	m_start_pts(0),
	m_file_format(nullptr),
	m_stream_format(nullptr),
	m_next_thumbnail_pts(0),
	m_packet(nullptr),
	m_video_stream_index(-1),
	m_stream(nullptr),
	m_codec(nullptr),
	m_frame(nullptr)
{
	int ret = avformat_open_input(&m_file_format, file_path.c_str(), nullptr, nullptr);

	if (ret < 0)
	{
		throw std::runtime_error("Cannot open video: " + ffmpegError(ret));
	}

	m_stream_format = m_file_format;
	ret = avformat_find_stream_info(m_stream_format, nullptr);

	if (ret < 0)
	{
		avformat_close_input(&m_file_format);
		throw std::runtime_error("Cannot read stream information: " + ffmpegError(ret));
	}

	const AVCodec* decoder = nullptr;
	const int videoStream = av_find_best_stream(m_stream_format, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);

	if (videoStream < 0)
	{
		avformat_close_input(&m_file_format);
		throw std::runtime_error("No video stream found");
	}
	m_video_stream_index = videoStream;
	std::cout << file_path << ": video stream " << videoStream << std::endl;

	m_stream = m_stream_format->streams[videoStream];
	m_codec = avcodec_alloc_context3(decoder);

	if (!m_codec)
	{
		avformat_close_input(&m_file_format);
		throw std::runtime_error("Cannot allocate codec context");
	}

	ret = avcodec_parameters_to_context(m_codec, m_stream->codecpar);

	if (ret < 0)
	{
		avcodec_free_context(&m_codec);
		avformat_close_input(&m_file_format);
		throw std::runtime_error(ffmpegError(ret));
	}

	std::cout << file_path << ": codec " << decoder->name << std::endl;
	ret = avcodec_open2(m_codec, decoder, nullptr);

	if (ret < 0)
	{
		avcodec_free_context(&m_codec);
		avformat_close_input(&m_file_format);
		throw std::runtime_error("Cannot open decoder: " + ffmpegError(ret));
	}

	m_packet = av_packet_alloc();
	m_frame = av_frame_alloc();

	if (!m_packet || !m_frame)
	{
		av_packet_free(&m_packet);
		av_frame_free(&m_frame);
		avcodec_free_context(&m_codec);
		avformat_close_input(&m_file_format);
		throw std::runtime_error("Cannot allocate packet/frame");
	}

	m_start_pts = (m_stream->start_time != AV_NOPTS_VALUE) ? m_stream->start_time : 0;
	m_duration = (m_stream_format->duration == AV_NOPTS_VALUE) ? 0 : m_stream_format->duration;
	m_stepsize = time_step(m_duration);
	m_next_thumbnail_pts = m_start_pts;
	std::cout << file_path << ": Duration: " << m_duration << " us, Interval: " << m_stepsize << " us" << std::endl;
}

double ThumbnailGenerator::duration(void) const
{
	return static_cast<double>(m_duration) / 1.0e6;
}

int64_t ThumbnailGenerator::time_step(const int64_t duration) const
{
	/* 1s           --> m_thumbnail_height */
	/* 4h = 4*60*60 --> 1000 pixels */

	const int64_t min_time = 1 * AV_TIME_BASE;
	const int64_t min_width = static_cast<int64_t>(m_thumbnail_height);
	const int64_t max_time = 4l * 60l * 60l * AV_TIME_BASE;
	const int64_t max_width = 1000;

	if (duration < min_time)
	{
		return 0;
	}

	const float width = static_cast<float>(min_width) +
	                    static_cast<float>(max_width - min_width) *
	                    logf(static_cast<float>(duration) / static_cast<float>(min_time)) /
	                    logf(static_cast<float>(max_time) / static_cast<float>(min_time));
	const float step = static_cast<float>(duration) / static_cast<float>(width);

	return static_cast<int64_t>(step);
}

void ThumbnailGenerator::process_frame(void)
{
	if ((m_frame->width <= 0) || (m_frame->height <= 0) || (m_frame->format == AV_PIX_FMT_NONE))
	{
		throw std::runtime_error("Decoded frame has invalid dimensions or pixel format");
	}

	const int64_t pts = m_frame->best_effort_timestamp;

	if (pts == AV_NOPTS_VALUE)
	{
		return;
	}

	// time position relative to the beginning of the video
	const int64_t elapsedUs = av_rescale_q(pts - m_start_pts, m_stream->time_base, AV_TIME_BASE_Q);

	if (elapsedUs < m_next_thumbnail_pts)
	{
		return;
	}

	// calculate thumbnail size
	// respect sample aspect ratio for non-square pixels
	const AVRational sar =
		m_frame->sample_aspect_ratio.num > 0
		    ? m_frame->sample_aspect_ratio
		    : m_stream->sample_aspect_ratio;

	const int width = std::max(
		1,
		static_cast<int>(std::lround(
							 static_cast<double>(m_frame->width) * av_q2d(sar) *
							 static_cast<double>(m_thumbnail_height) / static_cast<double>(m_frame->height))));

	// update scaling context
	m_sws = sws_getCachedContext(
		m_sws,
		m_frame->width,
		m_frame->height,
		static_cast<AVPixelFormat>(m_frame->format),
		width,
		static_cast<int>(m_thumbnail_height),
		AV_PIX_FMT_RGB24,
		SWS_POINT,    // SWS_FAST_BILINEAR, SWS_BILINEAR
		nullptr,
		nullptr,
		nullptr
		);

	if (!m_sws)
	{
		throw std::runtime_error("Cannot create scaling context");
	}

	// create thumbnail
	Thumbnail thumbnail;

	thumbnail.width = width;
	thumbnail.height = static_cast<int>(m_thumbnail_height);
	thumbnail.stride = width * 3;
	thumbnail.timestampMs = av_rescale_q(pts - m_start_pts, m_stream->time_base, AVRational{1, 1000});
	thumbnail.pixels.resize(static_cast<size_t>(thumbnail.stride) * m_thumbnail_height);

	uint8_t* dstData[4] = {
		thumbnail.pixels.data(),
		nullptr,
		nullptr,
		nullptr
	};

	int dstLinesize[4] = {thumbnail.stride, 0, 0, 0};

	// scale and convert to RGB24
	int scaledHeight = sws_scale(
		m_sws,
		m_frame->data,
		m_frame->linesize,
		0,
		m_frame->height,
		dstData,
		dstLinesize
		);

	if (static_cast<size_t>(scaledHeight) != m_thumbnail_height)
	{
		throw std::runtime_error("Thumbnail scaling failed");
	}

	m_thumbnails.push_back(std::move(thumbnail));

	// determine next thumbnail timestamp
	do
	{
		m_next_thumbnail_pts += m_stepsize;
	}
	while (m_next_thumbnail_pts <= elapsedUs);
}

int ThumbnailGenerator::receiveFrames(void)
{
	while (true)
	{
		int result = avcodec_receive_frame(m_codec, m_frame);

		if ((result == AVERROR(EAGAIN)) || (result == AVERROR_EOF))
		{
			return result;
		}

		if (result < 0)
		{
			throw std::runtime_error("Frame decoding failed: " + ffmpegError(result));
		}

		process_frame();

		av_frame_unref(m_frame);
	}
}

std::vector<Thumbnail> ThumbnailGenerator::generate(void)
{
	int status;

	while ((status = av_read_frame(m_stream_format, m_packet)) >= 0)
	{
		if (m_packet->stream_index == m_video_stream_index)
		{
			int ret = avcodec_send_packet(m_codec, m_packet);

			if (ret < 0)
			{
				av_packet_unref(m_packet);
				throw std::runtime_error("Packet submission failed: " + ffmpegError(ret));
			}

			receiveFrames();
		}

		av_packet_unref(m_packet);
	}

	if (status != AVERROR_EOF)
	{
		throw std::runtime_error("Video reading failed: " + ffmpegError(status));
	}

	status = avcodec_send_packet(m_codec, nullptr);

	if ((status < 0) && (status != AVERROR_EOF))
	{
		throw std::runtime_error("Decoder flush failed: " + ffmpegError(status));
	}

	receiveFrames();

	sws_freeContext(m_sws);
	av_packet_free(&m_packet);
	av_frame_free(&m_frame);
	avcodec_free_context(&m_codec);
	avformat_close_input(&m_file_format);
	return m_thumbnails;
}
