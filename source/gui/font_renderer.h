// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H

#include <string>
#include <vector>
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H

class FontRenderer
{
	private:
		FT_Library m_library;
		FT_Face m_face;

		FontRenderer(const FontRenderer&);
		FontRenderer& operator=(const FontRenderer&);

	public:
		FontRenderer(void);
		~FontRenderer(void);
		void load_font(const std::string& filename);
		void render_text(const std::string& text, const size_t font_height, std::vector<uint32_t>& image);
};

#endif
