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
#include <libavutil/error.h>
#include <libavutil/avutil.h>
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
		/** constructor. */
		explicit ThumbnailGenerator(void);

		/** generates thumbnails for a video file.
		 * @param filename path to the video file
		 * @return vector of generated thumbnails
		 */
		std::vector<Thumbnail> generate(const std::string& filename) const;

	private:
		const size_t m_thumbnail_height;   ///< height of the generated thumbnails in pixels

		/** determine thumbnail step size in seconds.
		 * Thumbnail interval is determined by the width of the video thumbnail.
		 * A video of 1 second duration should be m_thumbnail_height pixels wide.
		 * A video of 4 hours is 1000 pixels wide. Everything in between is interpolated logarithmically.
		 * All times are in microseconds.
		 * @param duration total video duration.
		 * @return thumbnail interval.
		 */
		int64_t time_step(const int64_t duration) const;

		/** processes a video frame and generates a thumbnail.
		 * @param frame the video frame to process
		 * @param stream the video stream
		 * @param startPts the start presentation timestamp
		 * @param nextThumbnailUs the next thumbnail timestamp in microseconds
		 * @param intervalUs the thumbnail interval in microseconds
		 * @param sws the scaling context
		 * @param thumbnails the vector to store generated thumbnails
		 */
		void process_frame(
			AVFrame* frame,
			AVStream* stream,
			const int64_t startPts,
			int64_t& nextThumbnailUs,
			const int64_t intervalUs,
			SwsContext& sws,
			std::vector<Thumbnail>& thumbnails) const;

		/** receives decoded video frames and generates thumbnails.
		 * @param codec the codec context
		 * @param frame the video frame to process
		 * @param stream the video stream
		 * @param startPts the start presentation timestamp
		 * @param nextThumbnailUs the next thumbnail timestamp in microseconds
		 * @param intervalUs the thumbnail interval in microseconds
		 * @param sws the scaling context
		 * @param thumbnails the vector to store generated thumbnails
		 * @return 0 if all frames have been processed, AVERROR(EAGAIN) if more packets are needed, or an error code
		 */
		int receiveFrames(
			AVCodecContext* codec,
			AVFrame* frame,
			AVStream* stream,
			const int64_t startPts,
			int64_t& nextThumbnailUs,
			const int64_t intervalUs,
			SwsContext& sws,
			std::vector<Thumbnail>& thumbnails) const;
};

#endif
