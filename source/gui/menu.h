// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MENU_H
#define MENU_H

#include "line_panel.h"
#include "util/openvr_interface.h"
#include "font_renderer.h"
#include "database/thumbnail_generator.h"
#include <map>

class Menu
{
	private:
		/** list of available menus. */
		typedef enum
		{
			MENU_NONE,             ///< no menu
			MENU_MAIN,             ///< main menu with player buttons
			MENU_TILING,           ///< tiling selection
			MENU_PROJECTION,       ///< projection selection
			MENU_SETTINGS,         ///< video settings menu
			MENU_FILE_MANAGER,     ///< file manager
			MENU_SELECT_AUDIO,     ///< audio track selection
			MENU_SELECT_SUBTITLE   ///< subtitle track selection
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
		std::map<std::string, size_t> m_preview_textures;
		struct preview_volume_t
		{
			preview_volume_t(void) :
				texture(0),
				size(0, 0, 0),
				duration(0.0f)
			{
			}
			size_t texture;
			glm::uvec3 size;
			float duration;
		};
		std::map<std::string, preview_volume_t> m_preview_volumes;
		const std::string menu_graphics_extension;

		void create_button_panel(const std::vector<action_t>& actions);
		void create_points(void);
		void list_directories(LinePanel& panel) const;
		void list_files(LinePanel& panel);
		Thumbnail load_thumbnail(const std::string& video_file) const;

		void main_menu(void);
		void tiling_menu(void);
		void projection_menu(void);
		void settings_menu(void);
		void select_audio_menu(void);
		void select_subtitle_menu(void);
		void file_menu(void);
		void handle_button_action(const action_t action);

	public:
		Menu(void);
		~Menu(void);

		bool active(void) const;

		void init(void);
		void clear(void);
		void set_playable(const bool playable);
		void checkMenuInteraction(const glm::mat4& controller, const glm::mat4& hmd, const OpenVRInterface::input_state_t& input);
		void draw(void) const;
};

#endif
