// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TOGGLE_BUTTON_H
#define TOGGLE_BUTTON_H

#include "action.h"
#include "simple_button.h"

class ToggleButton : public SimpleButton
{
	public:
		explicit ToggleButton(const action_t action, const std::string& image_name, const bool value);
		~ToggleButton(void) override;

		bool update_on_interaction(const Panel::intersection_t isec, const bool pressed, const bool released) override;
		void draw(void) const override;

	private:
		bool m_active;                    // flag for the current button state
};

#endif
