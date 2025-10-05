// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "menu.h"
#include "main.h"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// #define DEBUG_LINE std::cout << "########## " << __FILE__ << "(" << __LINE__ << "): " << __FUNCTION__ << "()" << std::endl

Menu::Menu(void) :
	m_button(),
	m_points(),
	m_submenu(MENU_NONE),
	m_hmd_pose(1.0)
{
}

void Menu::init(void)
{
	create_points();
}

void Menu::draw(void)
{
	switch (m_submenu)
	{
		case MENU_MAIN:
		case MENU_TILING:
		case MENU_PROJECTION:
			for (std::vector<Button>::iterator iter = m_button.begin(); iter != m_button.end(); ++iter)
			{
				iter->draw();
			}
			m_points.draw();
			break;
		case MENU_NONE:
		default:
			break;
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

void Menu::create_button_panel(const std::vector<Button::button_action_t>& actions)
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

	m_button.clear();
	m_button.resize(actions.size());

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

			m_button.at(i).init(0.5f, actions.at(i));
			m_button.at(i).set_transform(pose);
			i++;
		}
	}
}

void Menu::main_menu(void)
{
	const Projection& p = projection();
	Button::button_action_t action_tile;
	Button::button_action_t action_project = Button::BUTTON_PROJECT_SPHERE;

	switch (p.tiling())
	{
		case Projection::TILE_MONO:
			action_tile = Button::BUTTON_TILE_MONO;
			break;
		case Projection::TILE_LEFT_RIGHT:
			action_tile = Button::BUTTON_TILE_LEFT_RIGHT;
			break;
		case Projection::TILE_TOP_BOTTOM:
			action_tile = Button::BUTTON_TILE_TOP_BOTTOM;
			break;
		case Projection::TILE_CUBE_MAP_MONO:
			action_tile = Button::BUTTON_TILE_CUBE_MONO;
			break;
		case Projection::TILE_CUBE_MAP_STEREO:
			action_tile = Button::BUTTON_TILE_CUBE_STEREO;
			break;
		default:
			throw std::runtime_error("invalid video tiling");
	}

	switch (p.projection())
	{
		case Projection::PROJECTION_FLAT:
			action_project = Button::BUTTON_PROJECT_FLAT;
			break;
		case Projection::PROJECTION_CYLINDER:
			action_project = Button::BUTTON_PROJECT_CYLINDER;
			break;
		case Projection::PROJECTION_SPHERE:
			action_project = Button::BUTTON_PROJECT_SPHERE;
			break;
		case Projection::PROJECTION_FISHEYE:
			action_project = Button::BUTTON_PROJECT_FISHEYE;
			break;
		case Projection::PROJECTION_CUBE_MAP:
			action_project = Button::BUTTON_PROJECT_CUBE;
			break;
		default:
			throw std::runtime_error("invalid video projection");
	}

	m_submenu = MENU_MAIN;
	create_button_panel({
		Button::BUTTON_PLAY_PREVIOUS,
		Button::BUTTON_PLAY_BACKWARD,
		Button::BUTTON_PLAY_PLAY,
		Button::BUTTON_PLAY_FORWARD,
		Button::BUTTON_PLAY_NEXT,
		action_tile,
		action_project,
		Button::BUTTON_FILE_OPEN,
		Button::BUTTON_POWER
	});
}

void Menu::tiling_menu(void)
{
	m_submenu = MENU_TILING;
	create_button_panel({
		Button::BUTTON_TILE_MONO,
		Button::BUTTON_TILE_LEFT_RIGHT,
		Button::BUTTON_TILE_TOP_BOTTOM,
		Button::BUTTON_TILE_CUBE_MONO,
		Button::BUTTON_TILE_CUBE_STEREO
	});
}

void Menu::projection_menu(void)
{
	m_submenu = MENU_PROJECTION;
	create_button_panel({
		Button::BUTTON_PROJECT_FLAT,
		Button::BUTTON_PROJECT_CYLINDER,
		Button::BUTTON_PROJECT_SPHERE,
		Button::BUTTON_PROJECT_FISHEYE,
		Button::BUTTON_PROJECT_CUBE
	});
}

void Menu::handle_button_action(const Button::button_action_t action)
{
	switch (action)
	{
		case Button::BUTTON_FILE_DELETE:
			break;
		case Button::BUTTON_FILE_OPEN:
			break;
		case Button::BUTTON_PLAY_BACKWARD:
			player_backward();
			break;
		case Button::BUTTON_PLAY_FORWARD:
			player_foreward();
			break;
		case Button::BUTTON_PLAY_NEXT:
			player_next();
			break;
		case Button::BUTTON_PLAY_PAUSE:
			player_pause();
			break;
		case Button::BUTTON_PLAY_PLAY:
			player_play();
			break;
		case Button::BUTTON_PLAY_PREVIOUS:
			player_previous();
			break;
		case Button::BUTTON_POWER:
			m_submenu = MENU_NONE;
			quit();
			break;
		case Button::BUTTON_PROJECT_CUBE:

			if (m_submenu == MENU_MAIN)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_CUBE_MAP);
				main_menu();
			}
			break;
		case Button::BUTTON_PROJECT_CYLINDER:

			if (m_submenu == MENU_MAIN)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_CYLINDER);
				main_menu();
			}
			break;
		case Button::BUTTON_PROJECT_FISHEYE:

			if (m_submenu == MENU_MAIN)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_FISHEYE);
				main_menu();
			}
			break;
		case Button::BUTTON_PROJECT_FLAT:

			if (m_submenu == MENU_MAIN)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_FLAT);
				main_menu();
			}
			break;
		case Button::BUTTON_PROJECT_SPHERE:

			if (m_submenu == MENU_MAIN)
			{
				projection_menu();
			}
			else
			{
				projection().set_projection(Projection::PROJECTION_SPHERE);
				main_menu();
			}
			break;
		case Button::BUTTON_TILE_CUBE_MONO:

			if (m_submenu == MENU_MAIN)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_CUBE_MAP_MONO);
				main_menu();
			}
			break;
		case Button::BUTTON_TILE_CUBE_STEREO:

			if (m_submenu == MENU_MAIN)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_CUBE_MAP_STEREO);
				main_menu();
			}
			break;
		case Button::BUTTON_TILE_LEFT_RIGHT:

			if (m_submenu == MENU_MAIN)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_LEFT_RIGHT);
				main_menu();
			}
			break;
		case Button::BUTTON_TILE_MONO:

			if (m_submenu == MENU_MAIN)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_MONO);
				main_menu();
			}
			break;
		case Button::BUTTON_TILE_TOP_BOTTOM:

			if (m_submenu == MENU_MAIN)
			{
				tiling_menu();
			}
			else
			{
				projection().set_tiling(Projection::TILE_TOP_BOTTOM);
				main_menu();
			}
			break;
		default:
			break;
	}
}

void Menu::checkMenuInteraction(const glm::mat4& controller, const glm::mat4& hmd, const bool pressed)
{
	// activate menu, if disabled
	if (pressed && (m_submenu == MENU_NONE))
	{
		m_hmd_pose = hmd;
		main_menu();
		return;
	}

	bool buttonHit = false;
	std::vector<glm::vec3> intersections;
	for (std::vector<Button>::iterator iter = m_button.begin(); iter != m_button.end(); ++iter)
	{
		const Button::intersection_t isec = iter->intersection(controller);

		// draw point on panel
		if (isec.hit)
		{
			buttonHit |= isec.hit;
			// std::cout << "panel hit at (" << isec.global.x << ", " << isec.global.y << ", " << isec.global.z << ")" << std::endl;
			intersections.push_back(isec.global);
		}

		if (pressed && isec.hit)
		{
			// hit in rectangle local coords mapped to texture or 0..1 coords
			std::cout << "Controller " << " clicked button " << isec.button_id << " at local coords (u,v)=(" << isec.local.x << "," << isec.local.y << ")" << std::endl;
			handle_button_action(isec.action_id);

			// buttons may have been replaced by button action.
			// stop iteration over buttons of previous menu.
			break;
		}
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
	if (pressed && !buttonHit)
	{
		m_submenu = MENU_NONE;
	}
}
