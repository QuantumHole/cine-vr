// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "simple_button.h"
#include "main.h"

static const glm::vec4 button_color(0.0f, 0.0f, 0.0f, 0.0f);

const glm::vec2 SimpleButton::m_button_size(0.5f, 0.5f);    // size in scene coordinate space

SimpleButton::SimpleButton(const action_t action, const std::string& image_name) :
	Panel(action)
{
	init_shape(m_button_size, button_color);
	init_texture(image_name);
}

SimpleButton::~SimpleButton(void)
{
}

bool SimpleButton::update_on_interaction(const Panel::intersection_t isec, const bool, const bool released)
{
	if (released && isec.hit)
	{
		return true;
	}

	return false;
}

void SimpleButton::draw(void) const
{
	texture().bind();
	shape().draw();
	texture().unbind();
}
