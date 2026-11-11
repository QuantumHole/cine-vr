// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
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
		static FontRenderer* m_instance;

		FT_Library m_library;
		FT_Face m_face;

		FontRenderer(void);
		FontRenderer(const FontRenderer&);
		FontRenderer& operator=(const FontRenderer&);

	public:
		static const size_t font_size;

		static FontRenderer& instance(void);
		~FontRenderer(void);
		void load_font(const std::string& filename);
		void render_text(const std::string& text, std::vector<uint32_t>& image) const;
};

#endif
