// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SLIDE_BUTTON_H
#define SLIDE_BUTTON_H

#include "opengl/shape.h"
#include "opengl/texture.h"
#include "action.h"
#include "simple_button.h"

class SlideButton : public SimpleButton
{
	public:
		explicit SlideButton(const action_t action, const std::string& image_name, const float min, const float max, const float value);
		~SlideButton(void) override;

		float slide_value(void) const;
		bool update_on_interaction(const Panel::intersection_t isec, const OpenVRInterface::input_state_t& input) override;
		void draw(void) const override;

	private:
		bool m_active;                    // flag for ongoing interactions or visible slidebar
		const float m_slide_min;          // minimum slidebar value
		const float m_slide_max;          // maximum slidebar value
		float m_slide_pos;                // current slidebar value
		float m_slide_last;               // original slidebar value
		Shape m_slidebar;                 // slidebar shape when this button is slideable

		void init_slidebar(void);
};

#endif
