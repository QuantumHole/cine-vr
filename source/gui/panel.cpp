// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "panel.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

TTF_Font* Panel::m_font = nullptr;
size_t Panel::m_instances = 0;

Panel::Panel(void) :
	m_shape(),
	m_texture(),
	m_width(0),
	m_height(0)
{
}

Panel::~Panel(void)
{
	m_texture.remove();
	m_instances--;

	if (m_instances == 0)
	{
		TTF_CloseFont(m_font);
		m_font = nullptr;
	}
}

void Panel::init_area(const size_t width, const size_t height)
{
	if (m_width || m_height)
	{
		// reinitialize
		m_shape.remove();
		m_texture.remove();

		if (m_instances)
		{
			m_instances--;
		}
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

	if (m_font == nullptr)
	{
		m_font = TTF_OpenFont("/usr/share/fonts/TTF/OpenSans-Regular.ttf", 20.0);

		if (!m_font)
		{
			std::cout << __FUNCTION__ << " - SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
			throw std::runtime_error("invalid font");
		}
	}
	m_instances++;
}

void Panel::set_transform(const glm::mat4& pose)
{
	m_shape.set_transform(pose);
}

void Panel::text(const std::string& text, const int32_t x, const int32_t y)
{
	SDL_Color color = {0xff, 0xff, 0xff, 0xff};
	SDL_Surface* surface = TTF_RenderText_Blended(m_font, text.c_str(), text.size(), color);

	/* copy only the visible section of the surface */
	for (size_t i = 0; i < static_cast<size_t>(surface->h); i++)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0,
		                x,
		                y + static_cast<GLint>(i),
		                std::min(static_cast<GLsizei>(surface->w), static_cast<GLsizei>(std::max(0, static_cast<int32_t>(m_width) - x))),
		                1,
		                GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
		                reinterpret_cast<uint8_t*>(surface->pixels) + static_cast<ptrdiff_t>(static_cast<int>(i) * surface->pitch));
	}

	SDL_DestroySurface(surface);
}

void Panel::draw(void) const
{
	m_texture.bind();
	m_shape.draw();
	m_texture.unbind();
}
