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
		std::vector<uint8_t> m_pixels;

		std::string file_extension(const std::string& file_name) const;
		void load_bmp(const std::string& file_name);
		void load_tga(const std::string& file_name);
		void load_png(const std::string& file_name);
		void load_jpg(const std::string& file_name);

	public:
		explicit ImageFile(const std::string& file_name);

		const std::vector<uint8_t>& pixels(void) const;
		uint32_t width(void) const;
		uint32_t height(void) const;
		size_t bpp(void) const;
};

#endif
