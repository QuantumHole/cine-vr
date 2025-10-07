// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <stdint.h>
#include <string>
#include <vector>

class ImageFile
{
	private:
		uint32_t m_width;
		uint32_t m_height;
		uint16_t m_bits_per_pixel;
		size_t m_line_size;
		std::vector<uint8_t> m_pixels;

		std::string file_extension(const std::string& file_name) const;
		void flip(void);
		void load_bmp(const std::string& file_name);
		void load_tga(const std::string& file_name);
		void load_png(const std::string& file_name);
		void load_jpg(const std::string& file_name);

	public:
		explicit ImageFile(const std::string& file_name, const bool flip_y = false);
		explicit ImageFile(const size_t width,
		                   const size_t height,
		                   const size_t line_size,
		                   const uint8_t* data = nullptr);

		const std::vector<uint8_t>& pixels(void) const;
		uint32_t width(void) const;
		uint32_t height(void) const;
		bool has_alpha_channel(void) const;
		void paste(const ImageFile& frame, const size_t x, const size_t y);
		void mix(const uint8_t r, const uint8_t g, const uint8_t b, const float frac);
		void greyscale(void);
};

#endif
