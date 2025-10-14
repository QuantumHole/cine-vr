// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "button.h"
#include "main.h"
#include <iostream>

static const float slide_height = 5.0f;
static const glm::vec2 button_size(0.5f, 0.5f);    // size in scene coordinate space
static const glm::vec3 button_color(0.0f, 0.0f, 0.0f);

Button::Button(const action_t action, const std::string& image_name) :
	Panel(action),
	m_toggleable(false),
	m_slideable(false),
	m_active(true),
	m_slide_min(0.0f),
	m_slide_max(0.0f),
	m_slide_pos(0.0f),
	m_slide_last(0.0f),
	m_slidebar()
{
	init_shape(button_size, button_color);
	init_texture(image_name);
}

Button::Button(const action_t action, const std::string& image_name, const bool value) :
	Panel(action),
	m_toggleable(true),
	m_slideable(false),
	m_active(value),
	m_slide_min(0.0f),
	m_slide_max(0.0f),
	m_slide_pos(0.0f),
	m_slide_last(0.0f),
	m_slidebar()
{
	init_shape(button_size, button_color);
	init_texture(image_name);
}

Button::Button(const action_t action, const std::string& image_name, const float min, const float max, const float value) :
	Panel(action),
	m_toggleable(false),
	m_slideable(true),
	m_active(false),
	m_slide_min(min),
	m_slide_max(max),
	m_slide_pos(value),
	m_slide_last(value),
	m_slidebar()
{
	init_shape(button_size, button_color);
	init_texture(image_name);
	init_slidebar();
}

Button::~Button(void)
{
	m_slidebar.remove();
}

void Button::init_slidebar(void)
{
	const float eps = 1e-4f;
	const float y0 = 0.0f;
	const float y1 = slide_height * button_size.y;

	const std::vector<Vertex> svertices = {
		Vertex(glm::vec3(-0.4f * button_size.x, y0, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3(0.4f * button_size.x, y1, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.4f * button_size.x, y1, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3(0.4f * button_size.x, y0, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> sindices = {
		0, 1, 2,
		0, 3, 1,
	};

	m_slidebar.init_vertices(svertices, sindices, GL_TRIANGLES);
}

float Button::slide_value(void) const
{
	return m_slide_pos;
}

bool Button::update_on_interaction(const Panel::intersection_t isec, const bool pressed, const bool released)
{
	if (m_slideable)
	{
		if (isec.hit && pressed && !m_active)
		{
			m_slide_last = m_slide_pos;

			const float frac = (m_slide_pos - m_slide_min) / (m_slide_max - m_slide_min);
			const float y0 = -slide_height * button_size.y * frac;

			// move selection strip to current position
			glm::mat4 shifted_pose = glm::translate(Panel::pose(), glm::vec3(0.0f, y0, 0.0f));
			m_slidebar.set_transform(shifted_pose);
			m_active = true;
		}
		else if (!pressed && m_active)
		{
			shape().set_transform(Panel::pose());
			m_active = false;
		}
		else if (pressed && m_active)
		{
			float y = isec.local.y;
			y = (y * button_size.y) - 0.5f * button_size.y;    // global position

			// restrict y coordinate of shown icon from last slide position
			const float frac = (m_slide_last - m_slide_min) / (m_slide_max - m_slide_min);
			const float y0 = -slide_height * button_size.y * frac;
			const float y1 = slide_height * button_size.y * (1.0f - frac);
			const float global_y = std::min(std::max(y, y0), y1);
			y = (global_y - y0) / (y1 - y0);                   // relative position on slide bar

			m_slide_pos = m_slide_min + y * (m_slide_max - m_slide_min);

			// move button icon to cursor position
			glm::mat4 shifted_pose = glm::translate(Panel::pose(), glm::vec3(0.0f, global_y, 0.0f));
			shape().set_transform(shifted_pose);

			return true;
		}
	}

	if (released && isec.hit)
	{
		if (m_toggleable)
		{
			m_active = !m_active;
		}

		return true;
	}

	return false;
}

void Button::draw(void) const
{
	if (m_slideable && m_active)
	{
		m_slidebar.draw();
	}

	if (m_toggleable && !m_active)
	{
		shader().set_uniform("greyscale", true);
	}

	texture().bind();
	shape().draw();
	texture().unbind();

	if (m_toggleable && !m_active)
	{
		shader().set_uniform("greyscale", false);
	}
}
