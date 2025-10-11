// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "panel.h"
#include "font_renderer.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>

Panel::Panel(void) :
	m_shape(),
	m_texture(),
	m_width(0),
	m_height(0)
{
}

Panel::~Panel(void)
{
	m_shape.remove();
	m_texture.remove();
}

void Panel::init_area(const size_t width, const size_t height)
{
	if (m_width || m_height)
	{
		// reinitialize
		m_shape.remove();
		m_texture.remove();
	}

	m_width = width;
	m_height = height;
	glm::vec2 m_size(1.0f, 1.0f);

	// positions: rectangle in XY plane centered at 0, z=0
	const std::vector<Vertex> vertices = {
		Vertex(glm::vec3(-0.5f * m_size.x, -0.5f * m_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3(0.5f * m_size.x, 0.5f * m_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f * m_size.x, 0.5f * m_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3(0.5f * m_size.x, -0.5f * m_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> indices = {
		0, 1, 2,
		0, 3, 1,
	};

	m_shape.init_vertices(vertices, indices, GL_TRIANGLES);
	m_texture.init_dim(width, height, GL_TEXTURE_2D, 0);
}

void Panel::set_transform(const glm::mat4& pose)
{
	m_shape.set_transform(pose);
}

void Panel::text(const std::string& text, const int32_t x, const int32_t y)
{
	FontRenderer renderer;

	renderer.load_font("/usr/share/fonts/TTF/OpenSans-Regular.ttf");

	const size_t height = 20;
	std::vector<uint32_t> image;
	renderer.render_text(text, height, image);

	const size_t width = image.size() / height;
	/* copy only the visible section of the surface */
	for (size_t i = 0; i < height; i++)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0,
		                x,
		                y + static_cast<GLint>(i),
		                std::min(static_cast<GLsizei>(width), static_cast<GLsizei>(std::max(0, static_cast<int32_t>(m_width) - x))),
		                1,
		                GL_RGBA, GL_UNSIGNED_BYTE,
		                image.data() + static_cast<ptrdiff_t>(i * width));
	}
}

void Panel::draw(void) const
{
	m_texture.bind();
	m_shape.draw();
	m_texture.unbind();
}
