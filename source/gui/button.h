// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BUTTON_H
#define BUTTON_H

#include "opengl/shape.h"
#include "opengl/texture.h"
#include "action.h"
#include "panel.h"

class Button : public Panel
{
	public:
		explicit Button(const action_t action, const std::string& image_name);
		explicit Button(const action_t action, const std::string& image_name, const bool value);
		explicit Button(const action_t action, const std::string& image_name, const float min, const float max, const float value);
		~Button(void) override;

		float slide_value(void) const;
		bool update_on_interaction(const Panel::intersection_t isec, const bool pressed, const bool released) override;
		void draw(void) const override;

	private:
		const bool m_toggleable;          // enable on/off behaviour
		const bool m_slideable;           // enable slidebar behaviour
		bool m_active;                    // flag for possible interactions or visible slidebar
		const float m_slide_min;          // minimum slidebar value
		const float m_slide_max;          // maximum slidebar value
		float m_slide_pos;                // current slidebar value
		float m_slide_last;               // original slidebar value
		Shape m_slidebar;                 // slidebar shape when this button is slideable

		void init_slidebar(void);
};

#endif
