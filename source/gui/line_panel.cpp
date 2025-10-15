// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "line_panel.h"
#include "main.h"

#include <iostream>

static const glm::vec2 shape_size(3.0f, 5.0f);
static const glm::vec3 color(0.5f, 0.4f, 0.2f);
static const glm::uvec2 tex_size(300, 500);
static const size_t line_height = 20.0f;
static const size_t level_offset = 5;

LinePanel::LinePanel(const action_t action) :
	Panel(action),
	m_lines(),
	m_active_line(0),
	m_selection_bar()
{
	init_area(shape_size, color, tex_size);
}

LinePanel::~LinePanel(void)
{
}

void LinePanel::init_selection(void)
{
	const float eps = 1e-4f;
	const float y0 = 0.0f;
	const float y1 = shape_size.y * line_height / static_cast<float>(tex_size.y);

	const std::vector<Vertex> vertices = {
		Vertex(glm::vec3(-0.5f * shape_size.x, y0, -eps), glm::vec3(0.0f, 0.0f, -1.0f), 0.8f * color, glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.5f * shape_size.x, y1, -eps), glm::vec3(0.0f, 0.0f, -1.0f), 0.8f * color, glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f * shape_size.x, y1, -eps), glm::vec3(0.0f, 0.0f, -1.0f), 0.8f * color, glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.5f * shape_size.x, y0, -eps), glm::vec3(0.0f, 0.0f, -1.0f), 0.8f * color, glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> indices = {
		0, 1, 2,
		0, 3, 1,
	};

	m_selection_bar.init_vertices(vertices, indices, GL_TRIANGLES);
}

void LinePanel::draw(void) const
{
	shader().set_uniform("background", true);

	texture().bind();
	shape().draw();
	texture().unbind();

	shader().set_uniform("background", false);
}

bool LinePanel::update_on_interaction(const intersection_t isec, const bool, const bool released)
{
	const float line = (1.0f - isec.local.y) * static_cast<float>(tex_size.y) / line_height;
	if ((line > 0.0f) && (line < static_cast<float>(m_lines.size())))
	{
		m_active_line = static_cast<size_t>(line);
		std::cout << "active line: " << m_active_line << std::endl;
	}

	return released && isec.hit;
}

void LinePanel::add_line(const std::string& text, const std::string& content, const size_t level)
{
	const line_entry_t entry = {
		text,
		content.empty() ? text : content,
		level
	};

	Panel::text(text, static_cast<int32_t>(level * level_offset), static_cast<int32_t>(m_lines.size() * line_height));
	m_lines.push_back(entry);
}
