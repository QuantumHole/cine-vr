// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scroll_panel.h"
#include "application.h"

#include <stdexcept>
#include <algorithm>
#include <iostream>

ScrollPanel::ScrollPanel(const action_t action) :
	Panel(action),
	m_tex_offset(0, 0),
	m_tex_view(0, 0)
{
}

void ScrollPanel::set_view_size(const glm::uvec2& view)
{
	m_tex_view = view;
}

const glm::uvec2& ScrollPanel::texture_offset(void) const
{
	return m_tex_offset;
}

void ScrollPanel::reset_scroll(void)
{
	m_tex_offset = glm::uvec2(0, 0);
}

void ScrollPanel::draw(void) const
{
	const glm::uvec2& ts = tex_size();
	ShaderSet& shader = Application::instance().shader();

	if (ts.x && ts.y)
	{
		glm::vec2 offset = glm::vec2(m_tex_offset) / glm::vec2(ts);
		glm::vec2 scale  = glm::vec2(m_tex_view) / glm::vec2(ts);

		shader.set_uniform("texture_offset", offset);
		shader.set_uniform("texture_scale", scale);
	}

	Panel::draw();

	shader.set_uniform("texture_offset", glm::vec2(0.0f, 0.0f));
	shader.set_uniform("texture_scale", glm::vec2(1.0f, 1.0f));
}

bool ScrollPanel::update_on_interaction(const intersection_t isec, const OpenVRInterface::input_state_t& input)
{
	const float length = glm::length(input.pad.position);
	const unsigned int scroll_speed = 5;

	if (isec.hit && (length > 0.5f))
	{
		const glm::uvec2& ts = tex_size();
		const glm::uvec2 max_offset(ts.x > m_tex_view.x ? ts.x - m_tex_view.x : 0,
		                            ts.y > m_tex_view.y ? ts.y - m_tex_view.y : 0);

		if (input.pad.position.x > 0.5f)
		{
			m_tex_offset.x = std::min(m_tex_offset.x + scroll_speed, max_offset.x);
		}
		else if ((input.pad.position.x < -0.5f) && (m_tex_offset.x > 0))
		{
			m_tex_offset.x = (scroll_speed > m_tex_offset.x) ? 0 : (m_tex_offset.x - scroll_speed);
		}

		if ((input.pad.position.y > 0.5f) && (m_tex_offset.y > 0))
		{
			m_tex_offset.y = (scroll_speed > m_tex_offset.y) ? 0 : (m_tex_offset.y - scroll_speed);
		}
		else if (input.pad.position.y < -0.5f)
		{
			m_tex_offset.y = std::min(m_tex_offset.y + scroll_speed, max_offset.y);
		}
	}

	return false;
}
