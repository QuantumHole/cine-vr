// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "menu.h"
#include "main.h"
#include "util/file_system.h"
#include "simple_button.h"
#include "toggle_button.h"
#include "slide_button.h"
#include "progress_bar.h"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// #define DEBUG_LINE std::cout << "########## " << __FILE__ << "(" << __LINE__ << "): " << __FUNCTION__ << "()" << std::endl

Menu::Menu(void) :
	m_panel(),
	m_points(),
	m_submenu(MENU_NONE),
	m_hmd_pose(1.0),
	m_focus(ACTION_NONE),
	m_debounce(false),
	m_playable(false),
	m_current_directory("")
{
	FileSystem fs;

	m_current_directory = fs.current_directory();
}

Menu::~Menu(void)
{
	// delete previous buttons
	for (std::map<action_t, Panel*>::const_iterator iter = m_panel.begin(); iter != m_panel.end(); ++iter)
	{
		delete iter->second;
	}
}

bool Menu::active(void) const
{
	return m_submenu != MENU_NONE;
}

void Menu::init(void)
{
	create_points();
}

void Menu::set_playable(const bool playable)
{
	m_playable = playable;
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
			for (std::map<action_t, Panel*>::const_iterator iter = m_panel.begin(); iter != m_panel.end(); ++iter)
			{
				iter->second->draw();
			}
			m_points.draw();
			break;

		case MENU_NONE:
		default:
			break;
	}
}

void Menu::list_directories(LinePanel& panel) const
{
	FileSystem fs;
	std::vector<std::string> entries = fs.split_path(m_current_directory);

	panel.clear_lines();

	size_t i = 0;
	for (std::vector<std::string>::const_iterator iter = entries.begin(); iter != entries.end(); ++iter)
	{
		const std::string full = fs.join_path(entries.begin(), iter + 1);

		panel.add_line(iter->empty() ? full : (*iter), full, i);
		i++;
	}

	entries.push_back("");
	std::set<std::string> dirs = fs.directory_names(m_current_directory);
	for (std::set<std::string>::const_iterator iter = dirs.begin(); iter != dirs.end(); iter++)
	{
		entries[entries.size() - 1] = *iter;
		const std::string full = fs.join_path(entries.begin(), entries.end());
		panel.add_line(*iter, full, i);
	}
	panel.render_lines();
}

void Menu::list_files(LinePanel& panel) const
{
	FileSystem fs;
	std::set<std::string> entries = fs.file_names(m_current_directory);

	panel.clear_lines();

	for (std::set<std::string>::const_iterator iter = entries.begin(); iter != entries.end(); iter++)
	{
		const std::string full = m_current_directory + "/" + *iter;
		panel.add_line(*iter, full, 0);
	}
	panel.render_lines();
}

void Menu::create_points(void)
{
	const glm::vec4 point_color(0.2f, 1.0f, 0.2f, 1.0f);

	// simple line with two vertices, will update dynamically
	std::vector<Vertex> vertices = {
		Vertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), point_color, glm::vec2(0.0f, 0.0f)),
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

	for (std::map<action_t, Panel*>::const_iterator iter = m_panel.begin(); iter != m_panel.end(); ++iter)
	{
		delete iter->second;
	}
	m_panel.clear();

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

			Panel* b;
			switch (act)
			{
				case ACTION_BACK:
					b = new SimpleButton(ACTION_BACK, "images/back.png");
					break;
				case ACTION_DESKTOP:
					b = new SimpleButton(ACTION_DESKTOP, "images/desktop.png");
					break;
				case ACTION_DESKTOP_WINDOW:
					b = new SimpleButton(ACTION_DESKTOP_WINDOW, "images/window.png");
					break;
				case ACTION_FILE_DELETE:
					b = new SimpleButton(ACTION_FILE_DELETE, "images/delete.png");
					break;
				case ACTION_FILE_OPEN:
					b = new SimpleButton(ACTION_FILE_OPEN, "images/open.png");
					break;
				case ACTION_PLAY_BACKWARD:
					b = new SimpleButton(ACTION_PLAY_BACKWARD, "images/backward.png");
					break;
				case ACTION_PLAY_FORWARD:
					b = new SimpleButton(ACTION_PLAY_FORWARD, "images/forward.png");
					break;
				case ACTION_PLAY_NEXT:
					b = new SimpleButton(ACTION_PLAY_NEXT, "images/next.png");
					break;
				case ACTION_PLAY_PAUSE:
					b = new SimpleButton(ACTION_PLAY_PAUSE, "images/pause.png");
					break;
				case ACTION_PLAY_PLAY:
					b = new SimpleButton(ACTION_PLAY_PLAY, "images/play.png");
					break;
				case ACTION_PLAY_PREVIOUS:
					b = new SimpleButton(ACTION_PLAY_PREVIOUS, "images/previous.png");
					break;
				case ACTION_POWER:
					b = new SimpleButton(ACTION_POWER, "images/power.png");
					break;
				case ACTION_PROJECT_CUBE:
					b = new SimpleButton(ACTION_PROJECT_CUBE, "images/cube-mono.png");
					break;
				case ACTION_PROJECT_CYLINDER:
					b = new SimpleButton(ACTION_PROJECT_CYLINDER, "images/cylinder.png");
					break;
				case ACTION_PROJECT_FISHEYE:
					b = new SimpleButton(ACTION_PROJECT_FISHEYE, "images/fisheye.png");
					break;
				case ACTION_PROJECT_FLAT:
					b = new SimpleButton(ACTION_PROJECT_FLAT, "images/flat.png");
					break;
				case ACTION_PROJECT_SPHERE:
					b = new SimpleButton(ACTION_PROJECT_SPHERE, "images/sphere.png");
					break;
				case ACTION_SETTINGS:
					b = new SimpleButton(ACTION_SETTINGS, "images/settings.png");
					break;
				case ACTION_TILE_CUBE_MONO:
					b = new SimpleButton(ACTION_TILE_CUBE_MONO, "images/cube-mono.png");
					break;
				case ACTION_TILE_CUBE_STEREO:
					b = new SimpleButton(ACTION_TILE_CUBE_STEREO, "images/cube-stereo.png");
					break;
				case ACTION_TILE_LEFT_RIGHT:
					b = new SimpleButton(ACTION_TILE_LEFT_RIGHT, "images/left-right.png");
					break;
				case ACTION_TILE_MONO:
					b = new SimpleButton(ACTION_TILE_MONO, "images/mono.png");
					break;
				case ACTION_TILE_TOP_BOTTOM:
					b = new SimpleButton(ACTION_TILE_TOP_BOTTOM, "images/top-bottom.png");
					break;
				case ACTION_FLAG_MONO:
					b = new ToggleButton(act, "images/force-mono.png", projection().mono());
					break;
				case ACTION_FLAG_STRETCH:
					b = new ToggleButton(act, "images/stretch.png", projection().stretch());
					break;
				case ACTION_FLAG_SWITCH_EYES:
					b = new ToggleButton(act, "images/switch-eyes.png", projection().switch_eyes());
					break;
				case ACTION_PARAM_ANGLE:
					b = new SlideButton(act, "images/angle.png", 0.0f, 2.0f * glm::pi<float>(), projection().angle());
					break;
				case ACTION_PARAM_ZOOM:
					b = new SlideButton(act, "images/zoom.png", 0.0f, 10.0f, projection().zoom());
					break;
				case ACTION_VOLUME:
					b = new SlideButton(act, "images/volume.png", 0.0f, 100.0f, player().volume());
					break;
				default:
					throw std::runtime_error("invalid button ID");
			}

			b->set_transform(pose);
			m_panel[act] = b;
			i++;
		}
	}

	m_debounce = true;
}

void Menu::main_menu(void)
{
	m_submenu = MENU_MAIN;

	if (m_playable)
	{
		create_button_panel({
			ACTION_PLAY_PREVIOUS,
			ACTION_PLAY_BACKWARD,
			ACTION_PLAY_PLAY,
			ACTION_PLAY_FORWARD,
			ACTION_PLAY_NEXT,
			ACTION_VOLUME,
			ACTION_SETTINGS,
			ACTION_FILE_OPEN,
			ACTION_POWER
		});

		const float duration = player().duration();
		const float position = player().playtime();
		Panel* p = new ProgressBar(ACTION_PLAY_POSITION, "images/progress-bar.png", duration, position);
		glm::mat4 pose = glm::mat4(1.0f);
		pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
		pose = m_hmd_pose * pose;
		p->set_transform(pose);
		m_panel[ACTION_PLAY_POSITION] = p;
	}
	else
	{
		create_button_panel({
			ACTION_PLAY_PREVIOUS,
			ACTION_PLAY_NEXT,
			ACTION_SETTINGS,
			ACTION_FILE_OPEN,
			ACTION_POWER
		});
	}
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
	m_debounce = true;

	// delete previous buttons
	for (std::map<action_t, Panel*>::const_iterator iter = m_panel.begin(); iter != m_panel.end(); ++iter)
	{
		delete iter->second;
	}
	m_panel.clear();

	// back button
	const float rot_angle = 0.125f * glm::pi<float>();
	glm::mat4 pose = glm::mat4(1.0f);
	pose = glm::rotate(pose, -rot_angle, glm::vec3(1.0f, 0.0f, 0.0f));
	pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
	pose = m_hmd_pose * pose;

	action_t act = ACTION_BACK;
	Panel* b = new SimpleButton(act, "images/back.png");
	b->set_transform(pose);
	m_panel[act] = b;

	// desktop button
	pose = glm::mat4(1.0f);
	pose = glm::rotate(pose, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
	pose = m_hmd_pose * pose;

	act = ACTION_DESKTOP;
	b = new SimpleButton(act, "images/desktop.png");
	b->set_transform(pose);
	m_panel[act] = b;

	// window button
	pose = glm::mat4(1.0f);
	pose = glm::rotate(pose, -0.5f * rot_angle, glm::vec3(1.0f, 0.0f, 0.0f));
	pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
	pose = m_hmd_pose * pose;

	act = ACTION_DESKTOP_WINDOW;
	b = new SimpleButton(act, "images/window.png");
	b->set_transform(pose);
	m_panel[act] = b;

	// directory panel
	pose = glm::mat4(1.0f);
	pose = glm::rotate(pose, rot_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
	pose = m_hmd_pose * pose;

	LinePanel* p = new LinePanel(ACTION_DIRECTORY_SELECT, "directories");
	p->set_transform(pose);
	list_directories(*p);
	m_panel[ACTION_DIRECTORY_SELECT] = p;

	// file panel
	pose = glm::mat4(1.0f);
	pose = glm::rotate(pose, -rot_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
	pose = m_hmd_pose * pose;

	p = new LinePanel(ACTION_FILE_SELECT, "files");
	p->set_transform(pose);
	list_files(*p);
	m_panel[ACTION_FILE_SELECT] = p;
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
		case ACTION_DESKTOP:
			m_submenu = MENU_NONE;
			player_show_desktop();
			break;
		case ACTION_DESKTOP_WINDOW:
			m_submenu = MENU_NONE;
			break;
		case ACTION_DIRECTORY_SELECT:
		{
			LinePanel& panel = *dynamic_cast<LinePanel*>(m_panel.find(action)->second);
			m_current_directory = panel.get_selection();
			list_directories(panel);
			list_files(*dynamic_cast<LinePanel*>(m_panel.find(ACTION_FILE_SELECT)->second));
		}
		break;
		case ACTION_FILE_SELECT:
		{
			m_submenu = MENU_NONE;
			LinePanel& panel = *dynamic_cast<LinePanel*>(m_panel.find(action)->second);
			const std::string file_name = panel.get_selection();
			player_open_file(file_name);
		}
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
			player().pause();
			break;
		case ACTION_PLAY_PLAY:
			player().play();
			break;
		case ACTION_PLAY_POSITION:
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
			projection().set_angle(dynamic_cast<SlideButton*>(m_panel.find(action)->second)->slide_value());
			update_projection();
			break;
		case ACTION_PARAM_ZOOM:
			projection().set_zoom(dynamic_cast<SlideButton*>(m_panel.find(action)->second)->slide_value());
			update_projection();
			break;
		case ACTION_VOLUME:
			player().set_volume(dynamic_cast<SlideButton*>(m_panel.find(action)->second)->slide_value());
			break;
		default:
			break;
	}
}

void Menu::checkMenuInteraction(const glm::mat4& controller, const glm::mat4& hmd, const OpenVRInterface::input_state_t& input)
{
	// activate menu, if disabled
	if (input.trigger.button.released && (m_submenu == MENU_NONE))
	{
		m_hmd_pose = hmd;
		main_menu();
		return;
	}

	(void)m_focus;

	/* wait for button release after a new menu has been activated */
	if (m_debounce)
	{
		if (input.trigger.button.pressed || input.trigger.button.released)
		{
			return;
		}
		m_debounce = false;
		m_focus = ACTION_NONE;
	}

	/* check interactions with all active elements */
	bool buttonHit = false;
	std::vector<glm::vec3> intersections;
	std::set<action_t> active_actions;
	for (std::map<action_t, Panel*>::const_iterator iter = m_panel.begin(); iter != m_panel.end(); ++iter)
	{
		Panel* b = iter->second;
		const Panel::intersection_t isec = b->intersection(controller);

		// draw point on panel
		if (isec.hit)
		{
			buttonHit |= isec.hit;
			intersections.push_back(isec.global);
		}

		if (isec.hit && !input.trigger.button.pressed && !input.trigger.button.released)
		{
			m_focus = iter->first;
		}

		if ((m_focus == iter->first) && b->update_on_interaction(isec, input))
		{
			/* actions may change button set.
			 * Therefore, actions must not be processed,
			 * before all buttons have been iterated over
			 */
			active_actions.insert(iter->first);
		}
	}

	/* process activated actions */
	for (std::set<action_t>::iterator iter = active_actions.begin(); iter != active_actions.end(); ++iter)
	{
		handle_button_action(*iter);
	}

	// update scene objects
	// define all intersection points
	m_points.set_instances(intersections.size());
	for (size_t i = 0; i < intersections.size(); i++)
	{
		const glm::mat4 popo = glm::translate(glm::mat4(1.0f), intersections.at(i));
		m_points.set_transform(popo, i);
	}

	// deactivate menu, if no button was hit
	if (input.trigger.button.released && !buttonHit)
	{
		m_submenu = MENU_NONE;
		m_focus = ACTION_NONE;
	}
}
