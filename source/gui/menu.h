// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MENU_H
#define MENU_H

#include "button.h"
#include <map>

class Menu
{
	private:
		typedef enum
		{
			MENU_NONE,
			MENU_MAIN,
			MENU_TILING,
			MENU_PROJECTION
		}
		menu_t;

		std::map<Button::button_action_t, Button*> m_button;
		Shape m_points;
		menu_t m_submenu;
		glm::mat4 m_hmd_pose;

		void create_button_panel(const std::vector<Button::button_action_t>& actions);
		void create_points(void);

		void main_menu(void);
		void tiling_menu(void);
		void projection_menu(void);
		void handle_button_action(const Button::button_action_t action);

	public:
		Menu(void);
		void init(void);
		void checkMenuInteraction(const glm::mat4& controller, const glm::mat4& hmd, const bool released, const bool pressed);
		void draw(void) const;
};

#endif
