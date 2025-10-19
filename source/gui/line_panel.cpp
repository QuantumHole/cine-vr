// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "line_panel.h"
#include "main.h"

static const glm::vec2 shape_size(3.0f, 5.0f);
static const glm::vec4 panel_color(0.5f, 0.4f, 0.2f, 0.8f);
static const glm::vec4 selection_color(0.5f * panel_color);
static const glm::uvec2 texture_size(300, 500);
static const size_t line_height = 20;
static const size_t level_offset = line_height / 2;

LinePanel::LinePanel(const action_t action, const std::string& title) :
	Panel(action),
	m_title(title),
	m_lines(),
	m_active_line(0),
	m_title_bar(ACTION_NONE),
	m_selection_bar()
{
	init_area(shape_size, panel_color, texture_size);

	const glm::vec2 title_size(shape_size.x, line_height * shape_size.x / static_cast<float>(texture_size.x));
	m_title_bar.init_area(title_size, panel_color, glm::uvec2(texture_size.x, line_height));
	m_title_bar.text(m_title, 0, 0);

	init_text_bar(m_selection_bar);
}

LinePanel::~LinePanel(void)
{
	m_selection_bar.remove();
}

void LinePanel::set_transform(const glm::mat4& pose)
{
	// move title bar above selection panel
	const float y = 0.5f * shape_size.y + line_height * shape_size.x / static_cast<float>(texture_size.x);
	glm::mat4 shifted_pose = glm::translate(pose, glm::vec3(0.0f, y, 0.0f));

	m_title_bar.set_transform(shifted_pose);

	Panel::set_transform(pose);
}

void LinePanel::clear(void)
{
	m_lines.clear();
	Panel::clear();
}

const std::string LinePanel::get_selection(void) const
{
	if (m_active_line < m_lines.size())
	{
		return m_lines.at(m_active_line).content;
	}
	return "";
}

void LinePanel::init_text_bar(Shape& bar)
{
	const float eps = 1e-4f;
	const float y0 = 0.0f;
	const float y1 = shape_size.y * line_height / static_cast<float>(texture_size.y);

	const std::vector<Vertex> vertices = {
		Vertex(glm::vec3(-0.5f * shape_size.x, y0, eps), glm::vec3(0.0f, 0.0f, -1.0f), selection_color, glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.5f * shape_size.x, y1, eps), glm::vec3(0.0f, 0.0f, -1.0f), selection_color, glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f * shape_size.x, y1, eps), glm::vec3(0.0f, 0.0f, -1.0f), selection_color, glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.5f * shape_size.x, y0, eps), glm::vec3(0.0f, 0.0f, -1.0f), selection_color, glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> indices = {
		0, 1, 2,
		0, 3, 1,
	};

	bar.init_vertices(vertices, indices, GL_TRIANGLES);
}

void LinePanel::draw(void) const
{
	m_title_bar.draw();

	shader().set_uniform("background", true);

	texture().bind();
	shape().draw();
	texture().unbind();

	shader().set_uniform("background", false);

	if (m_active_line < m_lines.size())
	{
		m_selection_bar.draw();
	}
}

bool LinePanel::update_on_interaction(const intersection_t isec, const OpenVRInterface::input_state_t& input)
{
	if (isec.hit)
	{
		m_active_line = static_cast<size_t>((1.0f - isec.local.y) * static_cast<float>(texture_size.y) / line_height);

		const size_t num_lines = texture_size.y / line_height;
		float y = 0.5f * shape_size.y - static_cast<float>(m_active_line + 1) * shape_size.y / static_cast<float>(num_lines);

		// move selection bar to cursor position
		glm::mat4 shifted_pose = glm::translate(Panel::pose(), glm::vec3(0.0f, y, 0.0f));
		m_selection_bar.set_transform(shifted_pose);
	}
	else
	{
		m_active_line = m_lines.size();
	}

	return input.trigger.button.released && isec.hit;
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
	m_active_line = m_lines.size();    // no active selected line, count title line
}
