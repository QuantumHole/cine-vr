// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "progress_bar.h"
#include "main.h"
#include <iostream>

static const float tex2shape = 0.01f;

ProgressBar::ProgressBar(const action_t action, const std::string& image, const float max, const float pos) :
	Panel(action),
	m_progress_max(max),
	m_progress_pos(pos),
	m_cursor(),
	m_cursor_tex()
{
	init_texture(image);
	const glm::vec4 color(0.0f, 0.0f, 0.0f, 0.0f);
	const glm::uvec2 tex_size = texture().size();
	const glm::vec2 progress_size = tex2shape * glm::vec2(tex_size);
	init_shape(progress_size, color);
	init_cursor();
	set_cursor_position();
}

ProgressBar::~ProgressBar(void)
{
}

void ProgressBar::set_max(const float max)
{
	m_progress_max = max;
}

void ProgressBar::set_pos(const float pos)
{
	m_progress_pos = pos;
}

void ProgressBar::set_cursor_position(void)
{
	m_progress_pos = player().playtime();
	const float bar_size = tex2shape * static_cast<float>(texture().size().x);
	const float frac = m_progress_pos / m_progress_max;
	const float shift = (frac - 0.5f) * bar_size;

	// move cursor to current position
	glm::mat4 shifted_pose = glm::translate(Panel::pose(), glm::vec3(shift, 0.0f, 0.0f));
	m_cursor.set_transform(shifted_pose);
}

void ProgressBar::init_cursor(void)
{
	m_cursor_tex.init_image_file("images/progress-cursor.png", 0);

	const float eps = 1e-4f;
	// const float opacity = 0.8f;
	const glm::vec2 cursor_size = tex2shape * glm::vec2(m_cursor_tex.size());

	const std::vector<Vertex> svertices = {
		Vertex(glm::vec3(-0.5f * cursor_size.x, -0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.5f * cursor_size.x,  0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f * cursor_size.x,  0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.5f * cursor_size.x, -0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> sindices = {
		0, 1, 2,
		0, 3, 1,
	};

	m_cursor.init_vertices(svertices, sindices, GL_TRIANGLES);
}

void ProgressBar::draw(void) const
{
	texture().bind();
	shape().draw();
	texture().unbind();

	m_cursor_tex.bind();
	m_cursor.draw();
	m_cursor_tex.unbind();
}

bool ProgressBar::update_on_interaction(const intersection_t isec, const OpenVRInterface::input_state_t& input)
{
	set_cursor_position();
	return input.trigger.button.released && isec.hit;
}
