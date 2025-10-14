// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "menu.h"
#include "main.h"
#include "util/file_system.h"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// #define DEBUG_LINE std::cout << "########## " << __FILE__ << "(" << __LINE__ << "): " << __FUNCTION__ << "()" << std::endl

Menu::Menu(void) :
	m_button(),
	m_points(),
	m_submenu(MENU_NONE),
	m_hmd_pose(1.0),
	m_active_button(ACTION_NONE),
	m_debounce(false),
	m_panel_dir(),
	m_panel_file()
{
}

void Menu::init(void)
{
	create_points();
}

void Menu::draw(void) const
{
	switch (m_submenu)
	{
		case MENU_MAIN:
		case MENU_TILING:
		case MENU_PROJECTION:
		case MENU_SETTINGS:
		case MENU_FILE_MANAGER:
			for (std::map<action_t, Button*>::const_iterator iter = m_button.begin(); iter != m_button.end(); ++iter)
			{
				iter->second->draw();
			}
			m_points.draw();

			if (m_submenu == MENU_FILE_MANAGER)
			{
				m_panel_dir.draw();
				m_panel_file.draw();
			}
			break;

		case MENU_NONE:
		default:
			break;
	}
}

void Menu::list_directories(void) const
{
	m_panel_dir.text("Directories", 0, 0);

	FileSystem fs;
	const std::string current_dir = fs.current_directory();
	std::vector<std::string> entries = fs.split_path(current_dir);

	for (size_t i = 0; i < entries.size(); i++)
	{
		m_panel_dir.text(entries[i], static_cast<int32_t>(i) * 5, static_cast<int32_t>(i + 1) * 20);
	}

	const size_t num_ent = entries.size();
	std::set<std::string> dirs = fs.directory_names(current_dir);
	size_t i = 0;
	for (std::set<std::string>::const_iterator iter = dirs.begin(); iter != dirs.end(); iter++)
	{
		m_panel_dir.text(*iter, static_cast<int32_t>(num_ent) * 5, static_cast<int32_t>(num_ent + i + 1) * 20);
		i++;
	}
}

void Menu::list_files(void) const
{
	m_panel_file.text("Files", 0, 0);

	FileSystem fs;
	const std::string current_dir = fs.current_directory();
	std::set<std::string> entries = fs.file_names(current_dir);

	size_t i = 0;
	for (std::set<std::string>::const_iterator iter = entries.begin(); iter != entries.end(); iter++)
	{
		m_panel_file.text(*iter, 0, static_cast<int32_t>(i + 1) * 20);
		i++;
	}
}

void Menu::create_points(void)
{
	// simple line with two vertices, will update dynamically
	std::vector<Vertex> vertices = {
		Vertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.2f, 1.0f, 0.2f), glm::vec2(0.0f, 0.0f)),
	};
	const std::vector<GLuint> indices = {
		0,
	};

	m_points.init_vertices(vertices, indices, GL_POINTS);
}

void Menu::create_button_panel(const std::vector<action_t>& actions)
{
	// determine number of buttons in all rows
	std::vector<size_t> rows;
	size_t i = actions.size();

	while (i)
	{
		const size_t num = std::min<size_t>(5, i);
		rows.push_back(num);
		i -= num;
	}

	const float range_x = 0.25f * glm::pi<float>();
	const float step_xy = range_x / static_cast<float>(rows.at(0));

	for (std::map<action_t, Button*>::const_iterator iter = m_button.begin(); iter != m_button.end(); ++iter)
	{
		delete iter->second;
	}
	m_button.clear();

	i = 0;
	for (size_t y = 0; y < rows.size(); y++)
	{
		for (size_t x = 0; x < rows.at(y); x++)
		{
			const float angle_x = 0.5f * (static_cast<float>(rows.at(y)) * step_xy) - 0.5f * step_xy - static_cast<float>(x) * step_xy;
			const float angle_y = -step_xy * static_cast<float>(1 + y);

			glm::mat4 pose = glm::mat4(1.0f);
			pose = glm::rotate(pose, angle_x, glm::vec3(0.0f, 1.0f, 0.0f));
			pose = glm::rotate(pose, angle_y, glm::vec3(1.0f, 0.0f, 0.0f));
			pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
			pose = m_hmd_pose * pose;

			const action_t act = actions.at(i);

			Button* b;
			switch (act)
			{
				case ACTION_BACK:
					b = new Button(ACTION_BACK, "images/back.png");
					break;
				case ACTION_FILE_DELETE:
					b = new Button(ACTION_FILE_DELETE, "images/delete.png");
					break;
				case ACTION_FILE_OPEN:
					b = new Button(ACTION_FILE_OPEN, "images/open.png");
					break;
				case ACTION_PLAY_BACKWARD:
					b = new Button(ACTION_PLAY_BACKWARD, "images/backward.png");
					break;
				case ACTION_PLAY_FORWARD:
					b = new Button(ACTION_PLAY_FORWARD, "images/forward.png");
					break;
				case ACTION_PLAY_NEXT:
					b = new Button(ACTION_PLAY_NEXT, "images/next.png");
					break;
				case ACTION_PLAY_PAUSE:
					b = new Button(ACTION_PLAY_PAUSE, "images/pause.png");
					break;
				case ACTION_PLAY_PLAY:
					b = new Button(ACTION_PLAY_PLAY, "images/play.png");
					break;
				case ACTION_PLAY_PREVIOUS:
					b = new Button(ACTION_PLAY_PREVIOUS, "images/previous.png");
					break;
				case ACTION_POWER:
					b = new Button(ACTION_POWER, "images/power.png");
					break;
				case ACTION_PROJECT_CUBE:
					b = new Button(ACTION_PROJECT_CUBE, "images/cube-mono.png");
					break;
				case ACTION_PROJECT_CYLINDER:
					b = new Button(ACTION_PROJECT_CYLINDER, "images/cylinder.png");
					break;
				case ACTION_PROJECT_FISHEYE:
					b = new Button(ACTION_PROJECT_FISHEYE, "images/fisheye.png");
					break;
				case ACTION_PROJECT_FLAT:
					b = new Button(ACTION_PROJECT_FLAT, "images/flat.png");
					break;
				case ACTION_PROJECT_SPHERE:
					b = new Button(ACTION_PROJECT_SPHERE, "images/sphere.png");
					break;
				case ACTION_SETTINGS:
					b = new Button(ACTION_SETTINGS, "images/settings.png");
					break;
				case ACTION_TILE_CUBE_MONO:
					b = new Button(ACTION_TILE_CUBE_MONO, "images/cube-mono.png");
					break;
				case ACTION_TILE_CUBE_STEREO:
					b = new Button(ACTION_TILE_CUBE_STEREO, "images/cube-stereo.png");
					break;
				case ACTION_TILE_LEFT_RIGHT:
					b = new Button(ACTION_TILE_LEFT_RIGHT, "images/left-right.png");
					break;
				case ACTION_TILE_MONO:
					b = new Button(ACTION_TILE_MONO, "images/mono.png");
					break;
				case ACTION_TILE_TOP_BOTTOM:
					b = new Button(ACTION_TILE_TOP_BOTTOM, "images/top-bottom.png");
					break;
				case ACTION_FLAG_MONO:
					b = new Button(act, "images/force-mono.png", projection().mono());
					break;
				case ACTION_FLAG_STRETCH:
					b = new Button(act, "images/stretch.png", projection().stretch());
					break;
				case ACTION_FLAG_SWITCH_EYES:
					b = new Button(act, "images/switch-eyes.png", projection().switch_eyes());
					break;
				case ACTION_PARAM_ANGLE:
					b = new Button(act, "images/angle.png", 0.0f, 2.0f * glm::pi<float>(), projection().angle());
					break;
				case ACTION_PARAM_ZOOM:
					b = new Button(act, "images/zoom.png", 0.0f, 10.0f, projection().zoom());
					break;
				default:
					throw std::runtime_error("invalid button ID");
			}

			b->set_transform(pose);
			m_button[act] = b;
			i++;
		}
	}

	m_debounce = true;
}

void Menu::main_menu(void)
{
	m_submenu = MENU_MAIN;
	create_button_panel({
		ACTION_PLAY_PREVIOUS,
		ACTION_PLAY_BACKWARD,
		ACTION_PLAY_PLAY,
		ACTION_PLAY_FORWARD,
		ACTION_PLAY_NEXT,
		ACTION_SETTINGS,
		ACTION_FILE_OPEN,
		ACTION_POWER
	});
}

void Menu::tiling_menu(void)
{
	m_submenu = MENU_TILING;
	create_button_panel({
		ACTION_TILE_MONO,
		ACTION_TILE_LEFT_RIGHT,
		ACTION_TILE_TOP_BOTTOM,
		ACTION_TILE_CUBE_MONO,
		ACTION_TILE_CUBE_STEREO
	});
}

void Menu::projection_menu(void)
{
	m_submenu = MENU_PROJECTION;
	create_button_panel({
		ACTION_PROJECT_FLAT,
		ACTION_PROJECT_CYLINDER,
		ACTION_PROJECT_SPHERE,
		ACTION_PROJECT_FISHEYE,
		ACTION_PROJECT_CUBE
	});
}

void Menu::file_menu(void)
{
	m_submenu = MENU_FILE_MANAGER;
	create_button_panel({
		ACTION_BACK
	});

	const float rot_x = 0.125f * glm::pi<float>();

	glm::mat4 pose = glm::mat4(1.0f);
	pose = glm::rotate(pose, rot_x, glm::vec3(0.0f, 1.0f, 0.0f));
	pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
	pose = m_hmd_pose * pose;

	const glm::uvec2 panel_size(300, 500);
	m_panel_dir.init_area(panel_size.x, panel_size.y);
	m_panel_dir.set_transform(pose);
	list_directories();

	pose = glm::mat4(1.0f);
	pose = glm::rotate(pose, -rot_x, glm::vec3(0.0f, 1.0f, 0.0f));
	pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
	pose = m_hmd_pose * pose;

	m_panel_file.init_area(panel_size.x, panel_size.y);
	m_panel_file.set_transform(pose);
	list_files();
}

void Menu::settings_menu(void)
{
	const Projection& p = projection();
	action_t action_tile;
	action_t action_project = ACTION_PROJECT_SPHERE;

	switch (p.tiling())
	{
		case Projection::TILE_MONO:
			action_tile = ACTION_TILE_MONO;
			break;
		case Projection::TILE_LEFT_RIGHT:
			action_tile = ACTION_TILE_LEFT_RIGHT;
			break;
		case Projection::TILE_TOP_BOTTOM:
			action_tile = ACTION_TILE_TOP_BOTTOM;
			break;
		case Projection::TILE_CUBE_MAP_MONO:
			action_tile = ACTION_TILE_CUBE_MONO;
			break;
		case Projection::TILE_CUBE_MAP_STEREO:
			action_tile = ACTION_TILE_CUBE_STEREO;
			break;
		default:
			throw std::runtime_error("invalid video tiling");
	}

	switch (p.projection())
	{
		case Projection::PROJECTION_FLAT:
			action_project = ACTION_PROJECT_FLAT;
			break;
		case Projection::PROJECTION_CYLINDER:
			action_project = ACTION_PROJECT_CYLINDER;
			break;
		case Projection::PROJECTION_SPHERE:
			action_project = ACTION_PROJECT_SPHERE;
			break;
		case Projection::PROJECTION_FISHEYE:
			action_project = ACTION_PROJECT_FISHEYE;
			break;
		case Projection::PROJECTION_CUBE_MAP:
			action_project = ACTION_PROJECT_CUBE;
			break;
		default:
			throw std::runtime_error("invalid video projection");
	}

	m_submenu = MENU_SETTINGS;
	create_button_panel({
		action_tile,
		action_project,
		ACTION_FLAG_MONO,
		ACTION_FLAG_STRETCH,
		ACTION_FLAG_SWITCH_EYES,
		ACTION_PARAM_ANGLE,
		ACTION_PARAM_ZOOM,
		ACTION_BACK
	});
}

void Menu::handle_button_action(const action_t action)
{
	switch (action)
	{
		case ACTION_BACK:
			main_menu();
			break;
		case ACTION_FILE_DELETE:
			break;
		case ACTION_FILE_OPEN:
			file_menu();
			break;
		case ACTION_PLAY_BACKWARD:
			player_backward();
			break;
		case ACTION_PLAY_FORWARD:
			player_forward();
			break;
		case ACTION_PLAY_NEXT:
			player_next();
			break;
		case ACTION_PLAY_PAUSE:
			player_pause();
			break;
		case ACTION_PLAY_PLAY:
			player_play();
			break;
		case ACTION_PLAY_PREVIOUS:
			player_previous();
			break;
		case ACTION_POWER:
			m_submenu = MENU_NONE;
			quit();
			break;
		case ACTION_PROJECT_CUBE:

			if (m_submenu == MENU_SETTINGS)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_CUBE_MAP);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_PROJECT_CYLINDER:

			if (m_submenu == MENU_SETTINGS)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_CYLINDER);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_PROJECT_FISHEYE:

			if (m_submenu == MENU_SETTINGS)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_FISHEYE);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_PROJECT_FLAT:

			if (m_submenu == MENU_SETTINGS)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_FLAT);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_PROJECT_SPHERE:

			if (m_submenu == MENU_SETTINGS)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_SPHERE);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_SETTINGS:
			settings_menu();
			break;
		case ACTION_TILE_CUBE_MONO:

			if (m_submenu == MENU_SETTINGS)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_CUBE_MAP_MONO);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_TILE_CUBE_STEREO:

			if (m_submenu == MENU_SETTINGS)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_CUBE_MAP_STEREO);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_TILE_LEFT_RIGHT:

			if (m_submenu == MENU_SETTINGS)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_LEFT_RIGHT);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_TILE_MONO:

			if (m_submenu == MENU_SETTINGS)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_MONO);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_TILE_TOP_BOTTOM:

			if (m_submenu == MENU_SETTINGS)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_TOP_BOTTOM);
				update_projection();
				settings_menu();
			}
			break;
		case ACTION_FLAG_MONO:
			projection().set_mono(!projection().mono());
			update_projection();
			break;
		case ACTION_FLAG_STRETCH:
			projection().set_stretch(!projection().stretch());
			update_projection();
			break;
		case ACTION_FLAG_SWITCH_EYES:
			projection().set_switch_eyes(!projection().switch_eyes());
			update_projection();
			break;
		case ACTION_PARAM_ANGLE:
			projection().set_angle(m_button.find(action)->second->slide_value());
			update_projection();
			break;
		case ACTION_PARAM_ZOOM:
			projection().set_zoom(m_button.find(action)->second->slide_value());
			update_projection();
			break;
		default:
			break;
	}
}

void Menu::checkMenuInteraction(const glm::mat4& controller, const glm::mat4& hmd, const bool released, const bool pressed)
{
	// activate menu, if disabled
	if (released && (m_submenu == MENU_NONE))
	{
		m_hmd_pose = hmd;
		main_menu();
		return;
	}

	if (m_debounce)
	{
		if (pressed || released)
		{
			return;
		}
		m_debounce = false;
	}

	if (released)
	{
		m_active_button = ACTION_NONE;
	}

	bool buttonHit = false;
	std::vector<glm::vec3> intersections;
	for (std::map<action_t, Button*>::const_iterator iter = m_button.begin(); iter != m_button.end(); ++iter)
	{
		Button* b = iter->second;
		const Button::intersection_t isec = b->intersection(controller);

		// draw point on panel
		if (isec.hit)
		{
			buttonHit |= isec.hit;
			// std::cout << "panel hit at (" << isec.global.x << ", " << isec.global.y << ", " << isec.global.z << ")" << std::endl;
			intersections.push_back(isec.global);
		}

		if (((m_active_button == ACTION_NONE) || (m_active_button == iter->first)) && b->update_on_interaction(isec, pressed, released))
		{
			m_active_button = iter->first;
			handle_button_action(isec.action_id);

			// buttons may have been replaced by button action.
			// stop iteration over buttons of previous menu.
			break;
		}
	}

	(void)m_active_button;

	// update scene objects
	// define all intersection points
	m_points.set_instances(intersections.size());
	for (size_t i = 0; i < intersections.size(); i++)
	{
		const glm::mat4 popo = glm::translate(glm::mat4(1.0f), intersections.at(i));
		m_points.set_transform(popo, i);
	}

	// deactivate menu, if no button was hit
	if (released && !buttonHit)
	{
		m_submenu = MENU_NONE;
	}
}
