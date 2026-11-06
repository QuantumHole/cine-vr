// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef THUMBNAIL_GENERATOR_H
#define THUMBNAIL_GENERATOR_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

/** represents a thumbnail image */
struct Thumbnail
{
	int width = 0;                       ///< width of the image in pixels
	int height = 0;                      ///< height of the image in pixels
	int stride = 0;                      ///< stride of the image in bytes
	int64_t timestampMs = 0;             ///< time position of actual frame in ms
	std::vector<uint8_t> pixels = {};    ///< ARGB image data
};

/** generates thumbnail images from video files */
class ThumbnailGenerator
{
	public:
		/** constructor.
		 * @param file_path path to the video file
		 */
		explicit ThumbnailGenerator(const std::string& file_path);

		/** returns the duration of the video in seconds.
		 * @return video duration in seconds
		 */
		double duration(void) const;

		/** generates thumbnails for a video file.
		 * @return vector of generated thumbnails
		 */
		std::vector<Thumbnail> generate(void);

	private:
		const size_t m_thumbnail_height;     ///< height of the generated thumbnails in pixels
		const std::string m_file_path;       ///< path to the current video file
		int64_t m_duration;                  ///< duration of the video in microseconds
		int64_t m_stepsize;                  ///< step size for thumbnail generation in microseconds
		std::vector<Thumbnail> m_thumbnails; ///< vector to store generated thumbnails
		SwsContext* m_sws;                   ///< scaling context for converting frames to thumbnails
		int64_t m_start_pts;                 ///< start presentation timestamp of the video stream in microseconds
		AVFormatContext* m_file_format;      ///< format context for the video file
		AVFormatContext* m_stream_format;    ///< format context for the video stream
		int64_t m_next_thumbnail_pts;        ///< next presentation timestamp for thumbnail generation in microseconds
		AVPacket* m_packet;                  ///< packet for reading video frames
		int m_video_stream_index;            ///< index of the video stream in the format context
		AVStream* m_stream;                  ///< video stream
		AVCodecContext* m_codec;             ///< video codec
		AVFrame* m_frame;                    ///< buffer for one video frame

		/** copy constructor. */
		ThumbnailGenerator(const ThumbnailGenerator&) = delete;

		/** assignment operator. */
		ThumbnailGenerator& operator=(const ThumbnailGenerator&) = delete;

		/** determine thumbnail step size in seconds.
		 * Thumbnail interval is determined by the width of the video thumbnail.
		 * A video of 1 second duration should be m_thumbnail_height pixels wide.
		 * A video of 4 hours is 1000 pixels wide. Everything in between is interpolated logarithmically.
		 * All times are in microseconds.
		 * @param duration total video duration.
		 * @return thumbnail interval.
		 */
		int64_t time_step(const int64_t duration) const;

		/** processes a video frame and generates a thumbnail. */
		void process_frame(void);

		/** receives decoded video frames and generates thumbnails.
		 * @return 0 if all frames have been processed, AVERROR(EAGAIN) if more packets are needed, or an error code
		 */
		int receiveFrames(void);
};

#endif
