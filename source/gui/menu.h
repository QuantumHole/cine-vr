// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MENU_H
#define MENU_H

#include "line_panel.h"
#include "util/openvr_interface.h"
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

		std::map<action_t, Panel*> m_panel;
		Shape m_points;
		menu_t m_submenu;
		glm::mat4 m_hmd_pose;
		action_t m_focus;
		bool m_debounce;
		bool m_playable;
		std::string m_current_directory;

		void create_button_panel(const std::vector<action_t>& actions);
		void create_points(void);
		void list_directories(LinePanel& panel) const;
		void list_files(LinePanel& panel) const;

		void main_menu(void);
		void tiling_menu(void);
		void projection_menu(void);
		void settings_menu(void);
		void file_menu(void);
		void handle_button_action(const action_t action);

	public:
		Menu(void);
		~Menu(void);
		void init(void);
		void set_playable(const bool playable);
		void checkMenuInteraction(const glm::mat4& controller, const glm::mat4& hmd, const OpenVRInterface::input_state_t& input);
		void draw(void) const;
};

#endif
