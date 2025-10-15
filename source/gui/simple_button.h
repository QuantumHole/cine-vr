// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SIMPLE_BUTTON_H
#define SIMPLE_BUTTON_H

#include "opengl/shape.h"
#include "opengl/texture.h"
#include "action.h"
#include "panel.h"

class SimpleButton : public Panel
{
	protected:
		static const glm::vec2 m_button_size;

	public:
		explicit SimpleButton(const action_t action, const std::string& image_name);
		~SimpleButton(void) override;

		virtual bool update_on_interaction(const Panel::intersection_t isec, const bool pressed, const bool released) override;
		virtual void draw(void) const override;
};

#endif
