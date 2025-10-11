// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PANEL_H
#define PANEL_H

#include "opengl/shape.h"
#include "opengl/texture.h"
#include <vector>

class Panel
{
	private:
		Shape m_shape;
		Texture m_texture;
		size_t m_width;
		size_t m_height;

	public:
		explicit Panel(void);
		~Panel(void);
		void init_area(const size_t width, const size_t height);
		void set_transform(const glm::mat4& pose);
		void text(const std::string& text, const int32_t x = 0, const int32_t y = 0);
		void draw(void) const;
};

#endif
