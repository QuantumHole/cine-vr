// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "image_data.h"
#include "file_system.h"
#include <fstream>
#include <string.h>
#include <algorithm>
#include <png.h>
#include <jpeglib.h>
#include <math.h>

static uint16_t read_u16(const std::vector<uint8_t>& data, const size_t offset)
{
	return static_cast<uint16_t>(data[offset]) |
	       static_cast<uint16_t>(data[offset + 1] << 8);
}

static uint32_t read_u32(const std::vector<uint8_t>& data, const size_t offset)
{
	return static_cast<uint32_t>(data[offset]) |
	       (static_cast<uint32_t>(data[offset + 1]) << 8) |
	       (static_cast<uint32_t>(data[offset + 2]) << 16) |
	       (static_cast<uint32_t>(data[offset + 3]) << 24);
}

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

	if (!fs.is_file(file_name))
	{
		throw std::runtime_error("file not found: " + file_name);
	}

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
	hFile.read(reinterpret_cast<char*>(FileInfo.data()), static_cast<std::streamsize>(Length));

	if ((Length < 54) || (FileInfo[0] != 'B') || (FileInfo[1] != 'M'))
	{
		hFile.close();
		throw std::invalid_argument("Error: Invalid File Format. Bitmap Required.");
	}

	const uint32_t dib_size = read_u32(FileInfo, 14);

	if ((dib_size < 40) || (Length < 14 + dib_size))
	{
		hFile.close();
		throw std::invalid_argument("Error: Unsupported bitmap header.");
	}
	const int32_t signed_width = static_cast<int32_t>(read_u32(FileInfo, 18));
	const int32_t signed_height = static_cast<int32_t>(read_u32(FileInfo, 22));
	const uint16_t planes = read_u16(FileInfo, 26);
	const uint16_t bits_per_pixel = read_u16(FileInfo, 28);
	const uint32_t compression = read_u32(FileInfo, 30);
	const uint32_t pixel_offset = read_u32(FileInfo, 10);

	if ((signed_width <= 0) || (signed_height == 0) || (planes != 1) || (compression != 0) ||
	    !((bits_per_pixel == 1) || (bits_per_pixel == 4) || (bits_per_pixel == 8) ||
	      (bits_per_pixel == 24) || (bits_per_pixel == 32)))
	{
		hFile.close();
		throw std::invalid_argument("Unsupported bitmap encoding.");
	}
	m_width = static_cast<uint32_t>(signed_width);
	m_height = static_cast<uint32_t>(signed_height < 0 ? -static_cast<int64_t>(signed_height) : signed_height);
	const bool top_down = signed_height < 0;
	const uint32_t palette_count = read_u32(FileInfo, 46) == 0 ? (1u << bits_per_pixel) : read_u32(FileInfo, 46);
	const size_t palette_offset = 14 + dib_size;
	const bool indexed = bits_per_pixel <= 8;

	if (indexed && ((palette_offset + static_cast<size_t>(palette_count) * 4 > pixel_offset) ||
	                (pixel_offset > Length)))
	{
		hFile.close();
		throw std::invalid_argument("Invalid bitmap color table.");
	}
	const size_t row_stride = ((static_cast<size_t>(m_width) * bits_per_pixel + 31) / 32) * 4;

	if ((static_cast<size_t>(pixel_offset) + row_stride * m_height) > Length)
	{
		hFile.close();
		throw std::invalid_argument("Truncated bitmap pixel data.");
	}
	m_bits_per_pixel = 32;
	m_pixels.resize(static_cast<size_t>(m_width) * m_height * 4);
	for (uint32_t y = 0; y < m_height; ++y)
	{
		const uint32_t source_y = top_down ? y : (m_height - 1 - y);
		const size_t source_row = static_cast<size_t>(pixel_offset) + source_y * row_stride;
		for (uint32_t x = 0; x < m_width; ++x)
		{
			uint8_t red = 0;
			uint8_t green = 0;
			uint8_t blue = 0;
			uint8_t alpha = 255;

			if (indexed)
			{
				const size_t bit_index = static_cast<size_t>(x) * bits_per_pixel;
				const uint8_t packed = FileInfo[source_row + bit_index / 8];
				const uint32_t shift = 8 - bits_per_pixel - static_cast<uint32_t>(bit_index % 8);
				const uint32_t color_index = (packed >> shift) & ((1u << bits_per_pixel) - 1u);

				if (color_index >= palette_count)
				{
					hFile.close();
					throw std::invalid_argument("Invalid bitmap palette index.");
				}
				const size_t palette_entry = palette_offset + static_cast<size_t>(color_index) * 4;
				blue = FileInfo[palette_entry];
				green = FileInfo[palette_entry + 1];
				red = FileInfo[palette_entry + 2];
			}
			else
			{
				const size_t source_pixel = source_row + static_cast<size_t>(x) * (bits_per_pixel / 8);
				blue = FileInfo[source_pixel];
				green = FileInfo[source_pixel + 1];
				red = FileInfo[source_pixel + 2];

				if (bits_per_pixel == 32)
				{
					alpha = FileInfo[source_pixel + 3];
				}
			}
			const size_t destination = (static_cast<size_t>(y) * m_width + x) * 4;
			m_pixels[destination] = red;
			m_pixels[destination + 1] = green;
			m_pixels[destination + 2] = blue;
			m_pixels[destination + 3] = alpha;
		}
	}
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

	if ((m_bits_per_pixel == 8) && ((Header[2] == 3) || (Header[2] == 11)) && (Header[1] == 0))
	{
		m_pixels.resize(static_cast<size_t>(m_width) * m_height);
		hFile.seekg(18 + Header[0], std::ios::beg);

		if (Header[2] == 3)
		{
			hFile.read(reinterpret_cast<char*>(m_pixels.data()), static_cast<std::streamsize>(m_pixels.size()));
		}
		else
		{
			size_t written = 0;
			while (written < m_pixels.size())
			{
				uint8_t packet = 0;
				hFile.read(reinterpret_cast<char*>(&packet), 1);
				const size_t count = (packet & 0x7f) + 1;

				if (count > m_pixels.size() - written)
				{
					throw std::invalid_argument("Invalid grayscale TGA packet.");
				}

				if (packet & 0x80)
				{
					uint8_t gray = 0;
					hFile.read(reinterpret_cast<char*>(&gray), 1);
					std::fill(m_pixels.begin() + static_cast<ptrdiff_t>(written),
					          m_pixels.begin() + static_cast<ptrdiff_t>(written + count), gray);
				}
				else
				{
					hFile.read(reinterpret_cast<char*>(m_pixels.data() + written), static_cast<std::streamsize>(count));
				}
				written += count;
			}
		}
		hFile.close();
		return;
	}

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
