// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "toggle_button.h"
#include "main.h"

ToggleButton::ToggleButton(const action_t action, const std::string& image_name, const bool value) :
	SimpleButton(action, image_name),
	m_active(value)
{
}

ToggleButton::~ToggleButton(void)
{
}

bool ToggleButton::update_on_interaction(const Panel::intersection_t isec, const bool, const bool released)
{
	if (released && isec.hit)
	{
		m_active = !m_active;
		return true;
	}

	return false;
}

void ToggleButton::draw(void) const
{
	if (!m_active)
	{
		shader().set_uniform("greyscale", true);
	}

	texture().bind();
	shape().draw();
	texture().unbind();

	if (!m_active)
	{
		shader().set_uniform("greyscale", false);
	}
}
