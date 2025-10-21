// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "image_data.h"
#include "file_system.h"
#include <fstream>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <png.h>
#include <jpeglib.h>
#include <math.h>

ImageFile::ImageFile(const std::string& file_name) :
	m_width(0),
	m_height(0),
	m_bits_per_pixel(0),
	m_pixels()
{
	if (file_name.empty())
	{
		return;
	}
	FileSystem fs;
	const std::string ext = fs.extension(file_name);

	if (ext == "bmp")
	{
		load_bmp(file_name);
	}
	else if (ext == "tga")
	{
		load_tga(file_name);
	}
	else if (ext == "png")
	{
		load_png(file_name);
	}
	else if ((ext == "jpg") || (ext == "jpeg"))
	{
		load_jpg(file_name);
	}
	else
	{
		throw std::runtime_error("unknown image format");
	}
}

const std::vector<uint8_t>& ImageFile::pixels(void) const
{
	return m_pixels;
}

uint32_t ImageFile::width(void) const
{
	return m_width;
}

uint32_t ImageFile::height(void) const
{
	return m_height;
}

size_t ImageFile::bpp(void) const
{
	return m_bits_per_pixel;
}

void ImageFile::load_bmp(const std::string& file_name)
{
	std::fstream hFile(file_name, std::ios::in | std::ios::binary);

	if (!hFile.is_open())
	{
		throw std::invalid_argument("Error: File Not Found.");
	}

	hFile.seekg(0, std::ios::end);
	size_t Length = static_cast<size_t>(hFile.tellg());
	hFile.seekg(0, std::ios::beg);
	std::vector<uint8_t> FileInfo(Length);
	hFile.read(reinterpret_cast<char*>(FileInfo.data()), 54);

	if ((FileInfo[0] != 'B') && (FileInfo[1] != 'M'))
	{
		hFile.close();
		throw std::invalid_argument("Error: Invalid File Format. Bitmap Required.");
	}

	if ((FileInfo[28] != 24) && (FileInfo[28] != 32))
	{
		hFile.close();
		throw std::invalid_argument("Error: Invalid File Format. 24 or 32 bit Image Required.");
	}

	m_bits_per_pixel = FileInfo[28];
	m_width = static_cast<uint32_t>(FileInfo[18] + (FileInfo[19] << 8));
	m_height = static_cast<uint32_t>(FileInfo[22] + (FileInfo[23] << 8));
	uint32_t PixelsOffset = static_cast<uint32_t>(FileInfo[10] + (FileInfo[11] << 8));
	uint32_t size = ((m_width * m_bits_per_pixel + 31) / 32) * 4 * m_height;
	m_pixels.resize(size);

	hFile.seekg(PixelsOffset, std::ios::beg);
	hFile.read(reinterpret_cast<char*>(m_pixels.data()), size);
	hFile.close();
}

void ImageFile::load_tga(const std::string& file_name)
{
	typedef union
	{
		uint32_t Colour;
		struct
		{
			// TGA stores color components in BGR (or BGRA) format
			uint8_t B;
			uint8_t G;
			uint8_t R;
			uint8_t A;
		}
		cc;
	}
	pixel_t;

	std::fstream hFile(file_name, std::ios::in | std::ios::binary);

	if (!hFile.is_open())
	{
		throw std::invalid_argument("File Not Found.");
	}

	uint8_t Header[18] = {0};
	static uint8_t DeCompressed[12] = {0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	static uint8_t IsCompressed[12] = {0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

	hFile.read(reinterpret_cast<char*>(&Header), sizeof(Header));

	m_bits_per_pixel = Header[16];
	m_width  = Header[13] * 256 + Header[12];
	m_height = Header[15] * 256 + Header[14];
	const uint32_t m_size  = ((m_width * m_bits_per_pixel + 31) / 32) * 4 * m_height;

	if ((m_bits_per_pixel != 24) && (m_bits_per_pixel != 32))
	{
		hFile.close();
		throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit Image.");
	}

	m_pixels.resize(m_size);

	pixel_t Pixel = {0};
	size_t CurrentByte = 0;
	int BytesPerPixel = (m_bits_per_pixel / 8);

	if (!memcmp(DeCompressed, &Header, sizeof(DeCompressed)))
	{
		do
		{
			hFile.read(reinterpret_cast<char*>(&Pixel), BytesPerPixel);

			m_pixels[CurrentByte++] = Pixel.cc.R;
			m_pixels[CurrentByte++] = Pixel.cc.G;
			m_pixels[CurrentByte++] = Pixel.cc.B;

			if (m_bits_per_pixel > 24)
			{
				m_pixels[CurrentByte++] = Pixel.cc.A;
			}
		}
		while (CurrentByte < (m_width * m_height * static_cast < uint32_t > (BytesPerPixel)));
	}
	else if (!memcmp(IsCompressed, &Header, sizeof(IsCompressed)))
	{
		uint8_t ChunkHeader = {0};

		do
		{
			hFile.read(reinterpret_cast<char*>(&ChunkHeader), sizeof(ChunkHeader));

			if (ChunkHeader < 128)
			{
				++ChunkHeader;
				for (int I = 0; I < ChunkHeader; ++I)
				{
					hFile.read(reinterpret_cast<char*>(&Pixel), BytesPerPixel);

					m_pixels[CurrentByte++] = Pixel.cc.R;
					m_pixels[CurrentByte++] = Pixel.cc.G;
					m_pixels[CurrentByte++] = Pixel.cc.B;

					if (m_bits_per_pixel > 24)
					{
						m_pixels[CurrentByte++] = Pixel.cc.A;
					}
				}
			}
			else
			{
				ChunkHeader = static_cast<uint8_t>(ChunkHeader - 127);
				hFile.read(reinterpret_cast<char*>(&Pixel), BytesPerPixel);

				for (int I = 0; I < ChunkHeader; ++I)
				{
					m_pixels[CurrentByte++] = Pixel.cc.R;
					m_pixels[CurrentByte++] = Pixel.cc.G;
					m_pixels[CurrentByte++] = Pixel.cc.B;

					if (m_bits_per_pixel > 24)
					{
						m_pixels[CurrentByte++] = Pixel.cc.A;
					}
				}
			}
		}
		while (CurrentByte < (m_width * m_height * static_cast < uint32_t > (BytesPerPixel)));
	}
	else
	{
		hFile.close();
		throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit TGA File.");
	}

	hFile.close();
}

void ImageFile::load_png(const std::string& file_name)
{
	FILE* fp = fopen(file_name.c_str(), "rb");

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

	if (!png)
	{
		abort();
	}

	png_infop info = png_create_info_struct(png);

	if (!info)
	{
		abort();
	}

	if (setjmp(png_jmpbuf(png)))
	{
		abort();
	}

	png_init_io(png, fp);
	png_read_info(png, info);
	m_width = png_get_image_width(png, info);
	m_height = png_get_image_height(png, info);
	png_byte color_type = png_get_color_type(png, info);
	png_byte bit_depth = png_get_bit_depth(png, info);
	png_byte channels = png_get_channels(png, info);
	m_bits_per_pixel = static_cast<uint16_t>(bit_depth * channels);

	/* Read any color_type into 8bit depth, RGBA format. */
	/* See http://www.libpng.org/pub/png/libpng-manual.txt */
	if (bit_depth == 16)
	{
		png_set_strip_16(png);
	}

	if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(png);
	}

	/* PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth. */
	if ((color_type == PNG_COLOR_TYPE_GRAY) && (bit_depth < 8))
	{
		png_set_expand_gray_1_2_4_to_8(png);
	}

	if (png_get_valid(png, info, PNG_INFO_tRNS))
	{
		png_set_tRNS_to_alpha(png);
	}

	/* These color_type don't have an alpha channel then fill it with 0xff. */
	if ((color_type == PNG_COLOR_TYPE_RGB) ||
	    (color_type == PNG_COLOR_TYPE_GRAY) ||
	    (color_type == PNG_COLOR_TYPE_PALETTE))
	{
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	}

	if ((color_type == PNG_COLOR_TYPE_GRAY) ||
	    (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
	{
		png_set_gray_to_rgb(png);
	}
	png_read_update_info(png, info);

	const size_t row_bytes = png_get_rowbytes(png, info);

	m_pixels.resize(m_height * row_bytes);
	std::vector<png_bytep> row_pointers(m_height); // = (png_bytep*)malloc(sizeof(png_bytep) * height);

	for (unsigned int y = 0; y < m_height; y++)
	{
		row_pointers[y] = &m_pixels[y * row_bytes];
	}

	png_read_image(png, &row_pointers[0]);

	/* read updated info */
	bit_depth = png_get_bit_depth(png, info);
	channels = png_get_channels(png, info);
	m_bits_per_pixel = static_cast<uint16_t>(bit_depth * channels);

	fclose(fp);
}

void ImageFile::load_jpg(const std::string& file_name)
{
	FILE* file = fopen(file_name.c_str(), "rb");

	if (file == nullptr)
	{
		return;
	}

	struct jpeg_decompress_struct info;   // for our jpeg info
	struct jpeg_error_mgr err;            // the error handler

	info.err = jpeg_std_error(&err);
	jpeg_create_decompress(&info);        // fills info structure

	jpeg_stdio_src(&info, file);
	jpeg_read_header(&info, true);

	jpeg_start_decompress(&info);

	m_width = info.output_width;
	m_height = info.output_height;
	unsigned int numChannels = static_cast<unsigned int>(info.num_components); // 3 = RGB, 4 = RGBA
	m_bits_per_pixel = static_cast<uint16_t>(numChannels * 8);
	unsigned long dataSize = m_width * m_height * numChannels;

	// read RGB(A) scanlines one at a time into jdata[]

	m_pixels.resize(dataSize);
	// unsigned char *data = (unsigned char *)malloc(dataSize);
	unsigned char* rowptr;
	while (info.output_scanline < m_height)
	{
		const size_t index = info.output_scanline * m_width * numChannels;
		rowptr = &m_pixels[index];
		jpeg_read_scanlines(&info, &rowptr, 1);
	}

	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);

	fclose(file);
}
