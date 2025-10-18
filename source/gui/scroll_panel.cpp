// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scroll_panel.h"
#include "main.h"

#include <stdexcept>
#include <algorithm>

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

void ScrollPanel::draw(void) const
{
	const glm::uvec2& ts = tex_size();

	glm::vec2 offset(m_tex_offset.x / ts.x, m_tex_offset.y / ts.y);
	glm::vec2 scale(m_tex_view.x / ts.x, m_tex_view.y / ts.y);

	shader().set_uniform("texture_offset", offset);
	shader().set_uniform("texture_scale", scale);

	Panel::draw();

	shader().set_uniform("texture_offset", glm::vec2(0.0f, 0.0f));
	shader().set_uniform("texture_scale", glm::vec2(1.0f, 1.0f));
}

bool ScrollPanel::update_on_interaction(const intersection_t isec, const bool, const bool released)
{
	return released && isec.hit;
}
