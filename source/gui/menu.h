// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MENU_H
#define MENU_H

#include "button.h"
#include "panel.h"
#include <map>

class Menu
{
	private:
		typedef enum
		{
			MENU_NONE,
			MENU_MAIN,
			MENU_TILING,
			MENU_PROJECTION,
			MENU_SETTINGS,
			MENU_FILE_MANAGER
		}
		menu_t;

		std::map<action_t, Button*> m_button;
		Shape m_points;
		menu_t m_submenu;
		glm::mat4 m_hmd_pose;
		action_t m_active_button;
		bool m_debounce;
		Panel m_panel_dir;
		Panel m_panel_file;

		void create_button_panel(const std::vector<action_t>& actions);
		void create_points(void);
		void list_directories(void) const;
		void list_files(void) const;

		void main_menu(void);
		void tiling_menu(void);
		void projection_menu(void);
		void settings_menu(void);
		void file_menu(void);
		void handle_button_action(const action_t action);

	public:
		Menu(void);
		void init(void);
		void checkMenuInteraction(const glm::mat4& controller, const glm::mat4& hmd, const bool released, const bool pressed);
		void draw(void) const;
};

#endif
