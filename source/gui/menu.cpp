// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "menu.h"
#include "application.h"
#include "util/file_system.h"
#include "simple_button.h"
#include "toggle_button.h"
#include "slide_button.h"
#include "progress_bar.h"
#include "database/thumbnail_store.h"
#include "list_element.h"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

// #define DEBUG_LINE std::cout << "########## " << __FILE__ << "(" << __LINE__ << "): " << __FUNCTION__ << "()" << std::endl

Menu::Menu(void) :
	m_panel(),
	m_points(),
	m_submenu(MENU_NONE),
	m_hmd_pose(1.0),
	m_focus(ACTION_NONE),
	m_debounce(false),
	m_playable(false),
	m_current_directory(FileSystem().current_directory()),
	m_preview_textures(),
	m_preview_volumes(),
	m_database("cine-vr"),
	menu_graphics_extension(".svg")
{
}

Menu::~Menu(void)
{
	clear();
}

void Menu::clear(void)
{
	for (std::map<action_t, Panel*>::const_iterator iter = m_panel.begin(); iter != m_panel.end(); ++iter)
	{
		delete iter->second;
	}
	m_panel.clear();
	m_points.remove();
	m_submenu = MENU_NONE;
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

size_t Menu::create_thumbnail_label(const std::string& text, const bool align_right, const bool top_half) const
{
	std::vector<uint32_t> glyphs;

	FontRenderer::instance().render_text(text, glyphs);
	const size_t glyph_width = glyphs.size() / FontRenderer::font_size;
	const size_t visible_width = std::min(glyph_width, LinePanel::texture_width);
	const size_t texture_width = std::max<size_t>(1, visible_width);
	std::vector<uint8_t> image(texture_width * FontRenderer::font_size* sizeof(uint32_t), 0);

	if (visible_width > 0)
	{
		const uint8_t* glyph_data = reinterpret_cast<const uint8_t*>(glyphs.data());
		for (size_t row = 0; row < FontRenderer::font_size; row++)
		{
			const size_t source_offset = row * glyph_width * sizeof(uint32_t);
			const size_t target_offset = row * texture_width * sizeof(uint32_t);
			std::copy(glyph_data + source_offset,
			          glyph_data + source_offset + visible_width * sizeof(uint32_t),
			          image.begin() + static_cast<std::ptrdiff_t>(target_offset));
		}
	}

	RenderManager& renderer = RenderManager::instance();
	const size_t texture = renderer.add_texture(
		image,
		glm::uvec2(static_cast<GLuint>(texture_width), static_cast<GLuint>(FontRenderer::font_size)),
		GL_RGBA,
		0);
	const float top = top_half ? 0.5f : 0.0f;
	const float bottom = top_half ? 0.0f : -0.5f;
	const float display_width = 0.5f * static_cast<float>(visible_width) / static_cast<float>(LinePanel::texture_width);
	const float left = align_right ? 0.5f - display_width : -0.5f;
	const float right = left + display_width;
	const std::vector<Vertex> vertices = {
		Vertex(glm::vec3(left,  bottom, 0.001f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3(right, top,    0.001f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(left,  top,    0.001f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3(right, bottom, 0.001f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 1.0f)),
	};
	const std::vector<GLuint> indices = {0, 1, 2, 0, 3, 1};
	return renderer.create_shape_renderable(
		vertices,
		indices,
		std::vector<size_t>(1, texture),
		Application::instance().shader().id(),
		1,
		GL_TRIANGLES,
		false);
}

size_t Menu::create_list_element(const std::string& text, const size_t level, const Thumbnail* background, const std::string& duration, size_t* preview_texture) const
{
	if (preview_texture)
	{
		*preview_texture = 0;
	}

	const int64_t minimum_stride = background ? static_cast<int64_t>(background->width) * 3 : 0;
	const bool has_background = background &&
	                            (background->width > 0) && (background->height > 0) &&
	                            (static_cast<int64_t>(background->stride) == minimum_stride) &&
	                            (background->pixels.size() >= static_cast<size_t>(background->stride) *
	                             static_cast<size_t>(background->height));

	if (has_background)
	{
		RenderManager& renderer = RenderManager::instance();
		const size_t background_texture = renderer.add_texture(
			background->pixels,
			glm::uvec2(static_cast<GLuint>(background->width), static_cast<GLuint>(background->height)),
			GL_RGB,
			0);

		if (preview_texture)
		{
			*preview_texture = background_texture;
		}
		const std::vector<Vertex> background_vertices = {
			Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 1.0f)),
			Vertex(glm::vec3( 0.5f,  0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 0.0f)),
			Vertex(glm::vec3(-0.5f,  0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 0.0f)),
			Vertex(glm::vec3( 0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 1.0f)),
		};
		const std::vector<GLuint> background_indices = {0, 1, 2, 0, 3, 1};
		const size_t background_renderable = renderer.create_shape_renderable(
			background_vertices,
			background_indices,
			std::vector<size_t>(1, background_texture),
			Application::instance().shader().id(),
			1,
			GL_TRIANGLES,
			false);
		const size_t name_renderable = create_thumbnail_label(text, false, true);
		const size_t duration_renderable = create_thumbnail_label(duration, true, false);
		return renderer.create_group({background_renderable, name_renderable, duration_renderable}, false);
	}

	ListElement element;
	return element.create_text_element(text, level);
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

		const size_t renderable = create_list_element(iter->empty() ? full : (*iter), i);
		panel.add_line(renderable, full);
		i++;
	}

	entries.push_back("");
	std::set<std::string> dirs = fs.directory_names(m_current_directory);
	for (std::set<std::string>::const_iterator iter = dirs.begin(); iter != dirs.end(); iter++)
	{
		entries[entries.size() - 1] = *iter;
		const std::string full = fs.join_path(entries.begin(), entries.end());
		const size_t renderable = create_list_element(*iter, i);
		panel.add_line(renderable, full);
	}
	panel.render_lines();
}

void Menu::list_files(LinePanel& panel)
{
	FileSystem fs;
	std::set<std::string> entries = fs.file_names(m_current_directory);

	panel.clear_lines();
	m_preview_textures.clear();

	ListElement element;
	for (std::set<std::string>::const_iterator iter = entries.begin(); iter != entries.end(); iter++)
	{
		const std::string full = m_current_directory + "/" + *iter;
		const std::string ext = fs.extension(full);
		size_t renderable = 0;

		if (fs.is_video(ext))
		{
			ThumbnailStore store(full);
			Thumbnail thumbnail = store.get_thumbnail(-1.0f);
			size_t preview_texture = 0;

			if ((thumbnail.width > 0) && (thumbnail.height > 0) && !thumbnail.pixels.empty())
			{
				float duration = 0.0f;
				m_database.read_video(full, duration);
				const unsigned int total_seconds = static_cast<unsigned int>(std::max(0.0f, duration));
				const unsigned int hours = total_seconds / 3600;
				const unsigned int minutes = (total_seconds / 60) % 60;
				const unsigned int seconds = total_seconds % 60;
				std::ostringstream duration_text;
				duration_text << std::setfill('0') << std::setw(2) << hours << ":"
				              << std::setw(2) << minutes << ":" << std::setw(2) << seconds;
				renderable = create_list_element(*iter, 0, &thumbnail, duration_text.str(), &preview_texture);
				m_preview_textures[full] = preview_texture;
			}
			else
			{
				renderable = create_list_element(*iter, 0);
			}
		}
		else
		{
			renderable = create_list_element(*iter, 0);
		}

		panel.add_line(renderable, full);
	}
	panel.render_lines();
}

Thumbnail Menu::load_thumbnail(const std::string& video_file) const
{
	ThumbnailStore store(video_file);

	return store.get_thumbnail(-1.0f);
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
			Projection& proj = Application::instance().projection();
			switch (act)
			{
				case ACTION_BACK:
					b = new SimpleButton(ACTION_BACK, "images/back" + menu_graphics_extension);
					break;
				case ACTION_DESKTOP:
					b = new SimpleButton(ACTION_DESKTOP, "images/desktop" + menu_graphics_extension);
					break;
				case ACTION_DESKTOP_WINDOW:
					b = new SimpleButton(ACTION_DESKTOP_WINDOW, "images/window" + menu_graphics_extension);
					break;
				case ACTION_FILE_DELETE:
					b = new SimpleButton(ACTION_FILE_DELETE, "images/delete" + menu_graphics_extension);
					break;
				case ACTION_FILE_OPEN:
					b = new SimpleButton(ACTION_FILE_OPEN, "images/open" + menu_graphics_extension);
					break;
				case ACTION_PLAY_BACKWARD:
					b = new SimpleButton(ACTION_PLAY_BACKWARD, "images/backward" + menu_graphics_extension);
					break;
				case ACTION_PLAY_FORWARD:
					b = new SimpleButton(ACTION_PLAY_FORWARD, "images/forward" + menu_graphics_extension);
					break;
				case ACTION_PLAY_NEXT:
					b = new SimpleButton(ACTION_PLAY_NEXT, "images/next" + menu_graphics_extension);
					break;
				case ACTION_PLAY_PAUSE:
					b = new SimpleButton(ACTION_PLAY_PAUSE, "images/pause" + menu_graphics_extension);
					break;
				case ACTION_PLAY_PLAY:
					b = new SimpleButton(ACTION_PLAY_PLAY, "images/play" + menu_graphics_extension);
					break;
				case ACTION_PLAY_PREVIOUS:
					b = new SimpleButton(ACTION_PLAY_PREVIOUS, "images/previous" + menu_graphics_extension);
					break;
				case ACTION_POWER:
					b = new SimpleButton(ACTION_POWER, "images/power" + menu_graphics_extension);
					break;
				case ACTION_PROJECT_CUBE:
					b = new SimpleButton(ACTION_PROJECT_CUBE, "images/cube-mono" + menu_graphics_extension);
					break;
				case ACTION_PROJECT_CYLINDER:
					b = new SimpleButton(ACTION_PROJECT_CYLINDER, "images/cylinder" + menu_graphics_extension);
					break;
				case ACTION_PROJECT_FISHEYE:
					b = new SimpleButton(ACTION_PROJECT_FISHEYE, "images/fisheye" + menu_graphics_extension);
					break;
				case ACTION_PROJECT_FLAT:
					b = new SimpleButton(ACTION_PROJECT_FLAT, "images/flat" + menu_graphics_extension);
					break;
				case ACTION_PROJECT_SPHERE:
					b = new SimpleButton(ACTION_PROJECT_SPHERE, "images/sphere" + menu_graphics_extension);
					break;
				case ACTION_SETTINGS:
					b = new SimpleButton(ACTION_SETTINGS, "images/settings" + menu_graphics_extension);
					break;
				case ACTION_TILE_CUBE_MONO:
					b = new SimpleButton(ACTION_TILE_CUBE_MONO, "images/cube-mono" + menu_graphics_extension);
					break;
				case ACTION_TILE_CUBE_STEREO:
					b = new SimpleButton(ACTION_TILE_CUBE_STEREO, "images/cube-stereo" + menu_graphics_extension);
					break;
				case ACTION_TILE_LEFT_RIGHT:
					b = new SimpleButton(ACTION_TILE_LEFT_RIGHT, "images/left-right" + menu_graphics_extension);
					break;
				case ACTION_TILE_MONO:
					b = new SimpleButton(ACTION_TILE_MONO, "images/mono" + menu_graphics_extension);
					break;
				case ACTION_TILE_TOP_BOTTOM:
					b = new SimpleButton(ACTION_TILE_TOP_BOTTOM, "images/top-bottom" + menu_graphics_extension);
					break;
				case ACTION_FLAG_MONO:
					b = new ToggleButton(act, "images/force-mono" + menu_graphics_extension, proj.mono());
					break;
				case ACTION_FLAG_STRETCH:
					b = new ToggleButton(act, "images/stretch" + menu_graphics_extension, proj.stretch());
					break;
				case ACTION_FLAG_SWITCH_EYES:
					b = new ToggleButton(act, "images/switch-eyes" + menu_graphics_extension, proj.switch_eyes());
					break;
				case ACTION_PARAM_ANGLE:
					b = new SlideButton(act, "images/angle" + menu_graphics_extension, 0.0f, 2.0f * glm::pi<float>(), proj.angle());
					break;
				case ACTION_PARAM_ZOOM:
					b = new SlideButton(act, "images/zoom" + menu_graphics_extension, 0.0f, 10.0f, proj.zoom());
					break;
				case ACTION_VOLUME:
					b = new SlideButton(act, "images/volume" + menu_graphics_extension, 0.0f, 100.0f, Application::instance().player().volume());
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
		Player& player = Application::instance().player();
		create_button_panel({
			ACTION_PLAY_PREVIOUS,
			ACTION_PLAY_BACKWARD,
			player.is_playing() ? ACTION_PLAY_PAUSE : ACTION_PLAY_PLAY,
			ACTION_PLAY_FORWARD,
			ACTION_PLAY_NEXT,
			ACTION_VOLUME,
			ACTION_SETTINGS,
			ACTION_FILE_OPEN,
			ACTION_POWER
		});

		const float duration = player.duration();
		const float position = player.playtime();
		const std::map<std::string, size_t>::const_iterator preview_iter =
			m_preview_textures.find(Application::instance().current_file_name());
		const size_t preview_texture = (preview_iter != m_preview_textures.end()) ? preview_iter->second : 0;
		const std::string current_file = Application::instance().current_file_name();
		preview_volume_t& preview_volume = m_preview_volumes[current_file];

		if ((preview_volume.texture == 0) && (preview_iter != m_preview_textures.end()))
		{
			const ThumbnailVolume volume = ThumbnailStore(current_file).get_thumbnail_volume();

			if (!volume.timestamps.empty())
			{
				RenderManager& renderer = RenderManager::instance();
				preview_volume.texture = renderer.create_texture();
				preview_volume.timestamps = volume.timestamps;
				renderer.texture(preview_volume.texture).init_data_3d(
					volume.pixels,
					glm::uvec3(volume.size.x, volume.size.y, static_cast<GLuint>(volume.timestamps.size())),
					GL_RGB,
					1);
			}
		}
		Panel* p = new ProgressBar(
			ACTION_PLAY_POSITION,
			"images/progress-bar" + menu_graphics_extension,
			"images/progress-cursor" + menu_graphics_extension,
			duration,
			position,
			preview_texture,
			preview_volume.texture,
			preview_volume.timestamps);
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
	Panel* b = new SimpleButton(act, "images/back" + menu_graphics_extension);
	b->set_transform(pose);
	m_panel[act] = b;

	// desktop button
	pose = glm::mat4(1.0f);
	pose = glm::rotate(pose, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
	pose = m_hmd_pose * pose;

	act = ACTION_DESKTOP;
	b = new SimpleButton(act, "images/desktop" + menu_graphics_extension);
	b->set_transform(pose);
	m_panel[act] = b;

	// window button
	pose = glm::mat4(1.0f);
	pose = glm::rotate(pose, -0.5f * rot_angle, glm::vec3(1.0f, 0.0f, 0.0f));
	pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
	pose = m_hmd_pose * pose;

	act = ACTION_DESKTOP_WINDOW;
	b = new SimpleButton(act, "images/window" + menu_graphics_extension);
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
	const Projection& p = Application::instance().projection();
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
	Application& app = Application::instance();
	Projection& proj = app.projection();

	switch (action)
	{
		case ACTION_BACK:
			main_menu();
			break;
		case ACTION_DESKTOP:
			m_submenu = MENU_NONE;
			app.player_show_desktop();
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
			app.player_open_file(file_name);
		}
		break;
		case ACTION_FILE_DELETE:
			break;
		case ACTION_FILE_OPEN:
			file_menu();
			break;
		case ACTION_PLAY_BACKWARD:
			app.player_backward();
			break;
		case ACTION_PLAY_FORWARD:
			app.player_forward();
			break;
		case ACTION_PLAY_NEXT:
			app.player_next();
			break;
		case ACTION_PLAY_PAUSE:
			app.player().pause();
			break;
		case ACTION_PLAY_PLAY:
			app.player().play();
			break;
		case ACTION_PLAY_POSITION:
			break;
		case ACTION_PLAY_PREVIOUS:
			app.player_previous();
			break;
		case ACTION_POWER:
			m_submenu = MENU_NONE;
			app.quit();
			break;
		case ACTION_PROJECT_CUBE:

			if (m_submenu == MENU_SETTINGS)
			{
				projection_menu();
			}
			else
			{
				app.projection().set_projection(Projection::PROJECTION_CUBE_MAP);
				app.update_projection();
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
				proj.set_projection(Projection::PROJECTION_CYLINDER);
				app.update_projection();
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
				proj.set_projection(Projection::PROJECTION_FISHEYE);
				app.update_projection();
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
				proj.set_projection(Projection::PROJECTION_FLAT);
				app.update_projection();
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
				proj.set_projection(Projection::PROJECTION_SPHERE);
				app.update_projection();
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
				proj.set_tiling(Projection::TILE_CUBE_MAP_MONO);
				app.update_projection();
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
				proj.set_tiling(Projection::TILE_CUBE_MAP_STEREO);
				app.update_projection();
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
				proj.set_tiling(Projection::TILE_LEFT_RIGHT);
				app.update_projection();
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
				proj.set_tiling(Projection::TILE_MONO);
				app.update_projection();
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
				proj.set_tiling(Projection::TILE_TOP_BOTTOM);
				app.update_projection();
				settings_menu();
			}
			break;
		case ACTION_FLAG_MONO:
			proj.set_mono(!proj.mono());
			app.update_projection();
			break;
		case ACTION_FLAG_STRETCH:
			proj.set_stretch(!proj.stretch());
			app.update_projection();
			break;
		case ACTION_FLAG_SWITCH_EYES:
			proj.set_switch_eyes(!proj.switch_eyes());
			app.update_projection();
			break;
		case ACTION_PARAM_ANGLE:
			proj.set_angle(dynamic_cast<SlideButton*>(m_panel.find(action)->second)->slide_value());
			app.update_projection();
			break;
		case ACTION_PARAM_ZOOM:
			proj.set_zoom(dynamic_cast<SlideButton*>(m_panel.find(action)->second)->slide_value());
			app.update_projection();
			break;
		case ACTION_VOLUME:
			app.player().set_volume(dynamic_cast<SlideButton*>(m_panel.find(action)->second)->slide_value());
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
		b->update_state();

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
