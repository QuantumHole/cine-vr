// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "font_renderer.h"

#include <stdexcept>
#include <stdint.h>
#include <sstream>

static uint8_t mask_byte(const char byte)
{
	return static_cast<uint8_t>(0xff & byte);
}

static void safe_advance(std::string::const_iterator& it, std::string::const_iterator end)
{
	++it;

	if (it == end)
	{
		throw std::runtime_error("Not enough space");
	}

	// check trailing sequence character
	if (!((mask_byte(*it) >> 6) == 0x2))
	{
		throw std::runtime_error("Incomplete UTF-8");
	}
}

static std::vector<uint32_t> utf8_to_codepoints(const std::string& text)
{
	const uint16_t SURROGATE_MIN  = 0xd800u;
	const uint16_t SURROGATE_MAX  = 0xdfffu;
	const uint32_t CODE_POINT_MAX = 0x0010ffffu;

	std::vector<uint32_t> result;
	const std::string::const_iterator end = text.end();

	for (std::string::const_iterator it = text.begin(); it != end; ++it)
	{
		uint32_t cp = mask_byte(*it);

		// Determine the sequence length based on the lead octet
		size_t length = 0;

		if (cp < 0x80)
		{
			length = 1;
		}
		else if ((cp >> 5) == 0x6)
		{
			length = 2;
			safe_advance(it, end);
			cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
		}
		else if ((cp >> 4) == 0xe)
		{
			length = 3;
			safe_advance(it, end);
			cp = ((cp << 12) & 0xffff) + ((mask_byte(*it) << 6) & 0xfff);
			safe_advance(it, end);
			cp += (*it) & 0x3f;
		}
		else if ((cp >> 3) == 0x1e)
		{
			length = 4;
			safe_advance(it, end);
			cp = ((cp << 18) & 0x1fffff) + ((mask_byte(*it) << 12) & 0x3ffff);
			safe_advance(it, end);
			cp += (mask_byte(*it) << 6) & 0xfff;
			safe_advance(it, end);
			cp += (*it) & 0x3f;
		}
		else
		{
			throw std::runtime_error("Invalid UTF-8 sequence");
		}

		// check valid code point
		if ((cp > CODE_POINT_MAX) || ((cp >= SURROGATE_MIN) && (cp <= SURROGATE_MAX)))
		{
			throw std::runtime_error("Invalid code point");
		}

		// check correct sequence length
		if (!(((cp < 0x80) && (length == 1)) ||
		      ((cp < 0x800) && (length == 2)) ||
		      ((cp < 0x10000) && (length == 3)) ||
		      ((cp >= 0x10000) && (length == 4))))
		{
			throw std::runtime_error("UTF-8 sequence too long");
		}

		result.push_back(cp);
	}

	return result;
}

static void draw_bitmap(const FT_Bitmap* bitmap,
                        const size_t x,
                        const size_t y,
                        const size_t font_height,
                        std::vector<uint32_t>& image)
{
	size_t x_max = static_cast<size_t>(x) + bitmap->width;
	size_t y_max = static_cast<size_t>(y) + bitmap->rows;
	const size_t image_width = image.size() / font_height;

	/* for simplicity, we assume that `bitmap->pixel_mode' */
	/* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

	size_t p = 0;

	for (size_t i = static_cast<size_t>(x); i < x_max; i++, p++)
	{
		size_t q = 0;
		for (size_t j = static_cast<size_t>(y); j < y_max; j++, q++)
		{
			if ((i >= image_width) || (j >= font_height))
			{
				continue;
			}

			const char c = bitmap->buffer[q * bitmap->width + p];
			const uint32_t pixel = c | (c << 8) | (c << 16) | (c << 24);
			image[j * image_width + i] |= pixel;
		}
	}
}

FontRenderer::FontRenderer(void) :
	m_library(nullptr),
	m_face(nullptr)
{
	FT_Error status = FT_Init_FreeType(&m_library);              /* initialize library */

	if (status)
	{
		throw std::runtime_error("failed loading FreeType library");
	}
}

FontRenderer::~FontRenderer(void)
{
	if (m_face)
	{
		FT_Done_Face(m_face);
		m_face = nullptr;
	}

	if (m_library)
	{
		FT_Done_FreeType(m_library);
		m_library = nullptr;
	}
}

void FontRenderer::load_font(const std::string& filename)
{
	/* create face object */
	FT_Error status = FT_New_Face(m_library, filename.c_str(), 0, &m_face);

	if (status)
	{
		throw std::runtime_error("failed loading font face");
	}
}

void FontRenderer::render_text(const std::string& text,
                               const size_t font_height,
                               std::vector<uint32_t>& image)
{
	/* use 50pt at 100dpi */
	FT_Error status = FT_Set_Pixel_Sizes(m_face, static_cast<FT_UInt>(font_height), 0);                /* set character size */

	if (status)
	{
		throw std::runtime_error("failed setting font size");
	}

	/* cmap selection omitted;                                        */
	/* for simplicity we assume that the font contains a Unicode cmap */

	FT_GlyphSlot slot = m_face->glyph;

	/* set up matrix */
	FT_Matrix matrix;                 /* transformation matrix */

	matrix.xx = 0x10000L;
	matrix.xy = 0;
	matrix.yx = 0;
	matrix.yy = 0x10000L;

	/* the pen position in 26.6 cartesian space coordinates; */
	FT_Vector pen;                    /* untransformed origin  */

	pen.x = 0;
	pen.y = 0;

	// convert text into list of Unicode Codepoints
	std::vector<uint32_t> codepoints = utf8_to_codepoints(text);

	/* determine width of text */
	size_t image_width = 0;

	for (size_t i = 0; i < codepoints.size(); i++)
	{
		FT_UInt glyph_index;
		glyph_index = FT_Get_Char_Index(m_face, codepoints[i]);

		if (glyph_index == 0)
		{
			std::stringstream s;
			s << "character not found: " << codepoints[i];
			throw std::runtime_error(s.str());
		}

		/* set transformation */
		FT_Set_Transform(m_face, &matrix, &pen);

		/* load glyph image into the slot (erase previous one) */
		status = FT_Load_Char(m_face, codepoints[i], FT_LOAD_RENDER);

		if (status)
		{
			continue;                 /* ignore errors */
		}
		image_width = static_cast<size_t>(slot->bitmap_left) + slot->bitmap.width;

		/* increment pen position */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}
	image.resize(font_height * image_width);

	/* draw text to bitmap */
	pen.x = 0;
	pen.y = 0;
	for (size_t i = 0; i < codepoints.size(); i++)
	{
		FT_UInt glyph_index;
		glyph_index = FT_Get_Char_Index(m_face, codepoints[i]);

		if (glyph_index == 0)
		{
			std::stringstream s;
			s << "character not found: " << codepoints[i];
			throw std::runtime_error(s.str());
		}

		/* set transformation */
		FT_Set_Transform(m_face, &matrix, &pen);

		/* load glyph image into the slot (erase previous one) */
		status = FT_Load_Char(m_face, codepoints[i], FT_LOAD_RENDER);

		if (status)
		{
			continue;                 /* ignore errors */
		}

		/* now, draw to our target surface (convert position) */
		draw_bitmap(&slot->bitmap,
		            static_cast<size_t>(slot->bitmap_left),
		            font_height - static_cast<size_t>(slot->bitmap_top) - 1,
		            font_height,
		            image);

		/* increment pen position */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}
}
