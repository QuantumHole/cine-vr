// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slide_button.h"
#include "application.h"
#include <cmath>

static const float slide_height = 5.0f;
static const float slide_value_epsilon = 1e-5f;

static float quantize_slide_value(const float value, const float min, const float max, const float step)
{
	const float clamped_value = std::min(std::max(value, min), max);
	if (step <= 0.0f)
	{
		return clamped_value;
	}
	return std::min(std::max(min + roundf((clamped_value - min) / step) * step, min), max);
}

SlideButton::SlideButton(const action_t action, const std::string& image_name, const float min, const float max, const float value, const float step) :
	SimpleButton(action, image_name),
	m_active(false),
	m_slide_min(min),
	m_slide_max(max),
	m_slide_step(step),
	m_slide_pos(quantize_slide_value(value, min, max, step)),
	m_slide_reported(m_slide_pos),
	m_slide_last(m_slide_pos),
	m_last_report_time(),
	m_slidebar()
{
	init_slidebar();
}

SlideButton::~SlideButton(void)
{
	m_slidebar.remove();
}

void SlideButton::init_slidebar(void)
{
	const float eps = 1e-4f;
	const float y0 = 0.0f;
	const float y1 = slide_height * m_button_size.y;
	const float opacity = 0.8f;

	const std::vector<Vertex> svertices = {
		Vertex(glm::vec3(-0.4f * m_button_size.x, y0, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 1.0f, opacity), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.4f * m_button_size.x, y1, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(1.0f, 0.0f, 0.0f, opacity), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.4f * m_button_size.x, y1, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(1.0f, 0.0f, 0.0f, opacity), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.4f * m_button_size.x, y0, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 1.0f, opacity), glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> sindices = {
		0, 1, 2,
		0, 3, 1,
	};

	m_slidebar.init_vertices(svertices, sindices, GL_TRIANGLES);
}

float SlideButton::slide_value(void) const
{
	return m_slide_pos;
}

bool SlideButton::update_on_interaction(const Panel::intersection_t isec, const OpenVRInterface::input_state_t& input)
{
	if (isec.hit && input.trigger.button.pressed && !m_active)
	{
		m_slide_last = m_slide_pos;

		const float frac = (m_slide_pos - m_slide_min) / (m_slide_max - m_slide_min);
		const float y0 = -slide_height * m_button_size.y * frac;

		// move selection strip to current position
		glm::mat4 shifted_pose = glm::translate(Panel::pose(), glm::vec3(0.0f, y0, 0.0f));
		m_slidebar.set_transform(shifted_pose);
		m_active = true;
	}
	else if (!input.trigger.button.pressed && m_active)
	{
		set_graphic_pose(Panel::pose());
		m_active = false;
		m_slide_reported = m_slide_pos;
		return true;
	}
	else if (input.trigger.button.pressed && m_active)
	{
		float y = isec.local.y;
		y = (y * m_button_size.y) - 0.5f * m_button_size.y;    // global position

		// restrict y coordinate of shown icon from last slide position
		const float frac = (m_slide_last - m_slide_min) / (m_slide_max - m_slide_min);
		const float y0 = -slide_height * m_button_size.y * frac;
		const float y1 = slide_height * m_button_size.y * (1.0f - frac);
		const float global_y = std::min(std::max(y, y0), y1);
		y = (global_y - y0) / (y1 - y0);                   // relative position on slide bar

		const float slide_pos = quantize_slide_value(m_slide_min + y * (m_slide_max - m_slide_min), m_slide_min, m_slide_max, m_slide_step);
		if (fabsf(slide_pos - m_slide_pos) > slide_value_epsilon)
		{
			m_slide_pos = slide_pos;
			const float snapped_frac = (m_slide_pos - m_slide_min) / (m_slide_max - m_slide_min);
			const float snapped_global_y = y0 + snapped_frac * (y1 - y0);

			glm::mat4 shifted_pose = glm::translate(Panel::pose(), glm::vec3(0.0f, snapped_global_y, 0.0f));
			set_graphic_pose(shifted_pose);
		}

		const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (fabsf(m_slide_pos - m_slide_reported) > slide_value_epsilon && now - m_last_report_time >= std::chrono::milliseconds(100))
		{
			m_slide_reported = m_slide_pos;
			m_last_report_time = now;
			return true;
		}

		return false;
	}

	return input.trigger.button.released && isec.hit;
}

void SlideButton::draw(void) const
{
	if (m_active)
	{
		m_slidebar.draw();
	}

	SimpleButton::draw();
}
