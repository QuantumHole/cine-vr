// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "line_panel.h"
#include "application.h"
#include "opengl/render_manager.h"

#include <algorithm>
#include <memory>

static const glm::vec2 shape_size(3.0f, 5.0f);
static const glm::vec4 panel_color(0.5f, 0.4f, 0.2f, 0.8f);
static const glm::vec4 selection_color(0.5f * panel_color);
static const size_t max_lines = 15;
const size_t LinePanel::line_height = 30;
const size_t LinePanel::texture_width = static_cast<size_t>(max_lines * LinePanel::line_height * shape_size.x / shape_size.y);

LinePanel::LinePanel(const action_t action, const std::string& title) :
	ScrollPanel(action),
	m_title(title),
	m_lines(),
	m_active_line(0),
	m_title_bar(ACTION_NONE),
	m_selection_bar()
{
	init_area(shape_size, panel_color, glm::uvec2(1, 1));   // needs at least one pixel to be drawn

	const glm::vec2 title_size(shape_size.x, line_height * shape_size.x / static_cast<float>(texture_width));
	m_title_bar.init_area(title_size, panel_color, glm::uvec2(texture_width, line_height));
	m_title_bar.text(m_title);

	init_text_bar(m_selection_bar);
	set_view_size(glm::uvec2(texture_width, line_height * max_lines));
}

LinePanel::~LinePanel(void)
{
	m_selection_bar.remove();
	RenderManager& renderer = RenderManager::instance();
	for (const line_entry_t& entry : m_lines)
	{
		renderer.delete_renderable(entry.renderable);
	}
}

void LinePanel::set_transform(const glm::mat4& pose)
{
	// move title bar above selection panel
	const float y = 0.5f * shape_size.y + line_height * shape_size.x / static_cast<float>(texture_width);
	glm::mat4 shifted_pose = glm::translate(pose, glm::vec3(0.0f, y, 0.0f));

	m_title_bar.set_transform(shifted_pose);

	Panel::set_transform(pose);
}

void LinePanel::clear_lines(void)
{
	m_lines.clear();
	m_active_line = 0;
	reset_scroll();
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
	const std::vector<Vertex> vertices = {
		Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), selection_color, glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.5f,  0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), selection_color, glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f,  0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), selection_color, glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), selection_color, glm::vec2(1.0f, 1.0f)),
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
	ScrollPanel::draw();

	const size_t offset = texture_offset().y;
	const size_t view_height = line_height * max_lines;
	const size_t content_height = m_lines.size() * line_height;

	if (content_height > offset)
	{
		const size_t visible_end = std::min(content_height, offset + view_height);
		const size_t first_row = offset / line_height;
		const size_t last_row = std::min(m_lines.size() - 1, (visible_end - 1) / line_height);
		RenderManager& renderer = RenderManager::instance();
		for (size_t i = first_row; i <= last_row; i++)
		{
			const line_entry_t& entry = m_lines[i];

			const size_t row_top = i * line_height;
			const size_t visible_top = std::max(row_top, offset);
			const size_t visible_bottom = std::min(row_top + line_height, visible_end);
			const float visible_fraction = static_cast<float>(visible_bottom - visible_top) /
			                               static_cast<float>(line_height);
			const float visible_center = 0.5f * static_cast<float>(visible_top + visible_bottom - 2 * offset);
			const float row_height = shape_size.y / static_cast<float>(max_lines);
			const float pixel_height = shape_size.y / static_cast<float>(view_height);
			const float y = 0.5f * shape_size.y - visible_center * pixel_height;
			glm::mat4 transform = glm::translate(pose(), glm::vec3(0.0f, y, 0.01f));
			transform = glm::scale(transform, glm::vec3(shape_size.x, row_height * visible_fraction, 1.0f));
			renderer.draw(entry.renderable, transform, visible_fraction);
		}
	}

	Application::instance().shader().activate();

	if (m_active_line < m_lines.size())
	{
		const size_t row_top = m_active_line * line_height;
		const size_t row_bottom = row_top + line_height;
		const size_t visible_end = std::min(content_height, offset + view_height);

		if ((row_top < visible_end) && (row_bottom > offset))
		{
			const size_t visible_top = std::max(row_top, offset);
			const size_t visible_bottom = std::min(row_bottom, visible_end);
			const float visible_fraction = static_cast<float>(visible_bottom - visible_top) /
			                               static_cast<float>(line_height);
			const float visible_center = 0.5f * static_cast<float>(visible_top + visible_bottom - 2 * offset);
			const float row_height = shape_size.y / static_cast<float>(max_lines);
			const float pixel_height = shape_size.y / static_cast<float>(view_height);
			const float y = 0.5f * shape_size.y - visible_center * pixel_height;
			glm::mat4 transform = glm::translate(pose(), glm::vec3(0.0f, y, 0.02f));
			transform = glm::scale(transform, glm::vec3(shape_size.x, row_height * visible_fraction, 1.0f));
			m_selection_bar.set_transform(transform);
			m_selection_bar.draw();
		}
	}
}

bool LinePanel::update_on_interaction(const intersection_t isec, const OpenVRInterface::input_state_t& input)
{
	ScrollPanel::update_on_interaction(isec, input);

	if (isec.hit)
	{
		// determine selected line entry
		const float local = 1.0f - isec.local.y;
		const size_t offset = texture_offset().y;
		const size_t pixel_line = static_cast<size_t>(local * line_height * max_lines) + offset;
		m_active_line = std::min(pixel_line / line_height, m_lines.size());
	}
	else
	{
		m_active_line = m_lines.size();
	}

	return input.trigger.button.released && isec.hit;
}

void LinePanel::add_line(const size_t renderable, const std::string& content)
{
	const line_entry_t entry = {
		renderable,
		content
	};

	m_lines.push_back(entry);
	m_active_line = m_lines.size();    // no active selected line
}

void LinePanel::render_lines(void)
{
	const size_t texture_height = std::max(m_lines.size() * line_height, line_height * max_lines);

	Panel::init_texture(glm::uvec2(texture_width, static_cast<GLuint>(texture_height)));
	Panel::set_transform(pose());
	Panel::clear();
	reset_scroll();

	/* do not repeat lines */
	texture().bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, reinterpret_cast<const GLfloat*>(&panel_color));
	texture().unbind();
}
