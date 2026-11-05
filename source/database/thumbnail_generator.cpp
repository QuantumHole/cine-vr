// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnail_generator.h"

// extern "C" {
// #include <libavformat/avformat.h>
// #include <libavcodec/avcodec.h>
// #include <libswscale/swscale.h>
// #include <libavutil/error.h>
// #include <libavutil/avutil.h>
// }

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

ThumbnailGenerator::ThumbnailGenerator(void) :
	m_thumbnail_height(128)
{
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

void ThumbnailGenerator::process_frame(
	AVFrame* frame,
	AVStream* stream,
	const int64_t startPts,
	int64_t& nextThumbnailUs,
	const int64_t intervalUs,
	SwsContext*& sws,
	std::vector<Thumbnail>& thumbnails) const
{
	if ((frame->width <= 0) || (frame->height <= 0) || (frame->format == AV_PIX_FMT_NONE))
	{
		throw std::runtime_error("Decoded frame has invalid dimensions or pixel format");
	}

	const int64_t pts = frame->best_effort_timestamp;

	if (pts == AV_NOPTS_VALUE)
	{
		return;
	}

	// time position relative to the beginning of the video
	const int64_t elapsedUs = av_rescale_q(pts - startPts, stream->time_base, AV_TIME_BASE_Q);

	if (elapsedUs < nextThumbnailUs)
	{
		return;
	}

	// calculate thumbnail size
	const int width = std::max(
		1,
		static_cast<int>(std::lround(
							 static_cast<double>(frame->width) *
							 static_cast<double>(m_thumbnail_height) / static_cast<double>(frame->height))));

	// update scaling context
	sws = sws_getCachedContext(
		sws,
		frame->width,
		frame->height,
		static_cast<AVPixelFormat>(frame->format),
		width,
		static_cast<int>(m_thumbnail_height),
		AV_PIX_FMT_RGB24,
		SWS_SCALE_POINT,    // SWS_FAST_BILINEAR, SWS_BILINEAR
		nullptr,
		nullptr,
		nullptr
		);

	if (!sws)
	{
		throw std::runtime_error("Cannot create scaling context");
	}

	// create thumbnail
	Thumbnail thumbnail;

	thumbnail.width = width;
	thumbnail.height = static_cast<int>(m_thumbnail_height);
	thumbnail.stride = width * 3;
	thumbnail.timestampMs = av_rescale_q(pts - startPts, stream->time_base, AVRational{1, 1000});
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
		sws,
		frame->data,
		frame->linesize,
		0,
		frame->height,
		dstData,
		dstLinesize
		);

	if (static_cast<size_t>(scaledHeight) != m_thumbnail_height)
	{
		throw std::runtime_error("Thumbnail scaling failed");
	}

	thumbnails.push_back(std::move(thumbnail));
	std::cout << "Generated thumbnail "
	          << thumbnails.size() << " at "
	          << (static_cast<double>(thumbnail.timestampMs) / 1000.0) << " s, size: "
	          << thumbnail.width << "x"
	          << thumbnail.height << std::endl;

	// determine next thumbnail timestamp
	do
	{
		nextThumbnailUs += intervalUs;
	}
	while (nextThumbnailUs <= elapsedUs);
}

int ThumbnailGenerator::receiveFrames(
	AVCodecContext* codec,
	AVFrame* frame,
	AVStream* stream,
	const int64_t startPts,
	int64_t& nextThumbnailUs,
	const int64_t intervalUs,
	SwsContext*& sws,
	std::vector<Thumbnail>& thumbnails) const
{
	while (true)
	{
		int result = avcodec_receive_frame(codec, frame);

		if ((result == AVERROR(EAGAIN)) || (result == AVERROR_EOF))
		{
			return result;
		}

		if (result < 0)
		{
			throw std::runtime_error("Frame decoding failed: " + ffmpegError(result));
		}

		process_frame(frame, stream, startPts, nextThumbnailUs, intervalUs, sws, thumbnails);

		av_frame_unref(frame);
	}
}

std::vector<Thumbnail> ThumbnailGenerator::generate(const std::string& filename) const
{
	AVFormatContext* rawFormat = nullptr;
	int ret = avformat_open_input(&rawFormat, filename.c_str(), nullptr, nullptr);

	if (ret < 0)
	{
		throw std::runtime_error("Cannot open video: " + ffmpegError(ret));
	}

	AVFormatContext* format = rawFormat;
	ret = avformat_find_stream_info(format, nullptr);

	if (ret < 0)
	{
		avformat_close_input(&rawFormat);
		throw std::runtime_error("Cannot read stream information: " + ffmpegError(ret));
	}

	const AVCodec* decoder = nullptr;
	const int videoStream = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);

	if (videoStream < 0)
	{
		avformat_close_input(&rawFormat);
		throw std::runtime_error("No video stream found");
	}
	std::cout << filename << ": video stream " << videoStream << std::endl;

	AVStream* stream = format->streams[videoStream];
	AVCodecContext* codec = avcodec_alloc_context3(decoder);

	if (!codec)
	{
		avformat_close_input(&rawFormat);
		throw std::runtime_error("Cannot allocate codec context");
	}

	ret = avcodec_parameters_to_context(codec, stream->codecpar);

	if (ret < 0)
	{
		avcodec_free_context(&codec);
		avformat_close_input(&rawFormat);
		throw std::runtime_error(ffmpegError(ret));
	}

	std::cout << filename << ": codec " << decoder->name << std::endl;
	ret = avcodec_open2(codec, decoder, nullptr);

	if (ret < 0)
	{
		avcodec_free_context(&codec);
		avformat_close_input(&rawFormat);
		throw std::runtime_error("Cannot open decoder: " + ffmpegError(ret));
	}

	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	if (!packet || !frame)
	{
		av_packet_free(&packet);
		av_frame_free(&frame);
		avcodec_free_context(&codec);
		avformat_close_input(&rawFormat);
		throw std::runtime_error("Cannot allocate packet/frame");
	}

	const int64_t startPts = (stream->start_time != AV_NOPTS_VALUE) ? stream->start_time : 0;
	int64_t nextThumbnailUs = 0;
	const int64_t durationUs = (format->duration == AV_NOPTS_VALUE) ? 0 : format->duration;
	const int64_t intervalUs = time_step(durationUs);
	std::cout << filename << ": Duration: " << durationUs << " us, Interval: " << intervalUs << " us" << std::endl;

	SwsContext* rawSws = nullptr;
	std::vector<Thumbnail> thumbnails;
	while ((ret = av_read_frame(format, packet)) >= 0)
	{
		if (packet->stream_index == videoStream)
		{
			ret = avcodec_send_packet(codec, packet);

			if (ret < 0)
			{
				av_packet_unref(packet);
				throw std::runtime_error("Packet submission failed: " + ffmpegError(ret));
			}

			receiveFrames(codec, frame, stream, startPts, nextThumbnailUs, intervalUs, rawSws, thumbnails);
		}

		av_packet_unref(packet);
	}

	if (ret != AVERROR_EOF)
	{
		throw std::runtime_error("Video reading failed: " + ffmpegError(ret));
	}

	ret = avcodec_send_packet(codec, nullptr);

	if ((ret < 0) && (ret != AVERROR_EOF))
	{
		throw std::runtime_error("Decoder flush failed: " + ffmpegError(ret));
	}

	receiveFrames(codec, frame, stream, startPts, nextThumbnailUs, intervalUs, rawSws, thumbnails);

	sws_freeContext(rawSws);
	av_packet_free(&packet);
	av_frame_free(&frame);
	avcodec_free_context(&codec);
	avformat_close_input(&rawFormat);
	return thumbnails;
}
