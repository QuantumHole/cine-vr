// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "progress_bar.h"
#include "application.h"
#include "util/file_system.h"
#include <iostream>
#include <stdexcept>

static const float tex2shape = 0.01f;

ProgressBar::ProgressBar(
	const action_t action,
	const std::string& imagebar,
	const std::string& imagecursor,
	const float max,
	const float pos) :
	Panel(action),
	m_progress_max(max),
	m_progress_pos(pos),
	m_renderer(RenderManager::instance()),
	m_background_renderable(0),
	m_cursor_renderable(0),
	m_background_shape_set(0),
	m_cursor_shape_set(0),
	m_cursor_texture(0),
	m_progress_size(0.0f, 0.0f),
	m_cursor_size(0.0f, 0.0f),
	m_background_transform(1.0f),
	m_cursor_transform(1.0f),
	m_background_uses_texture(false),
	m_cursor_uses_texture(false)
{
	const std::string extbar = FileSystem().extension(imagebar);

	if (FileSystem().is_image(extbar))
	{
		init_texture(imagebar);
		m_progress_size = tex2shape * glm::vec2(texture().size());
		m_background_uses_texture = true;
	}
	else if (FileSystem().is_vector(extbar))
	{
		m_background_shape_set = m_renderer.load_svg(imagebar);
		m_progress_size = tex2shape * glm::vec2(605.0f, 29.0f);
		const float svg_scale = 0.5f * m_progress_size.x;
		m_background_transform = glm::scale(glm::mat4(1.0f), glm::vec3(svg_scale, svg_scale, 1.0f));
	}
	else
	{
		throw std::runtime_error("unsupported image format: " + extbar);
	}

	if ((m_progress_size.x <= 0.0f) || (m_progress_size.y <= 0.0f))
	{
		throw std::runtime_error("invalid progress bar dimensions");
	}
	const glm::vec4 color(0.0f);
	init_shape(m_progress_size, color);

	if (m_background_uses_texture)
	{
		m_background_shape_set = Panel::shape_set_id();
	}
	std::vector<size_t> background_textures;

	if (m_background_uses_texture)
	{
		background_textures.push_back(texture_id());
	}

	const size_t shader_id = Application::instance().shader().id();
	m_background_renderable = m_renderer.create_shape_set_renderable(m_background_shape_set,
	                                                                 background_textures,
	                                                                 shader_id,
	                                                                 false);

	const std::string extcursor = FileSystem().extension(imagecursor);

	if (FileSystem().is_image(extcursor))
	{
		m_cursor_texture = m_renderer.create_texture();
		Texture& cursor_texture = m_renderer.texture(m_cursor_texture);
		cursor_texture.init_image_file(imagecursor, 0);
		m_cursor_size = tex2shape * glm::vec2(cursor_texture.size());
		m_cursor_uses_texture = true;
		m_cursor_texture = texture_id();
		init_cursor(m_cursor_size);
	}
	else if (FileSystem().is_vector(extcursor))
	{
		m_cursor_shape_set = m_renderer.load_svg(imagecursor);
		m_cursor_size.y = m_progress_size.y * (64.0f / 29.0f);
		m_cursor_size.x = m_cursor_size.y * (277.27591f / 832.0f);
		const float svg_scale = m_cursor_size.y * 0.5f;
		m_cursor_transform = glm::scale(glm::mat4(1.0f), glm::vec3(svg_scale, svg_scale, 1.0f));
		m_cursor_renderable = m_renderer.create_shape_set_renderable(m_cursor_shape_set,
		                                                             std::vector<size_t>(),
		                                                             shader_id,
		                                                             false);
	}
	else
	{
		throw std::runtime_error("unsupported image format: " + extcursor);
	}

	set_cursor_position();
}

ProgressBar::~ProgressBar(void)
{
	m_renderer.delete_renderable(m_background_renderable);
	m_renderer.delete_renderable(m_cursor_renderable);
}

void ProgressBar::set_transform(const glm::mat4& pose)
{
	Panel::set_transform(pose);
	m_renderer.set_shape_set_transform(m_background_shape_set, pose * m_background_transform);
	set_cursor_position();
}

void ProgressBar::set_max(const float max)
{
	m_progress_max = max;
}

void ProgressBar::set_pos(const float pos)
{
	m_progress_pos = pos;
}

void ProgressBar::set_cursor_position(void)
{
	if (m_progress_max <= 0.0f)
	{
		return;
	}
	m_progress_pos = Application::instance().player().playtime();
	const float frac = glm::clamp(m_progress_pos / m_progress_max, 0.0f, 1.0f);
	const float shift = (frac - 0.5f) * m_progress_size.x;

	// cursor hovers slightly above the progress bar to avoid z-fighting
	glm::mat4 shifted_pose = glm::translate(Panel::pose(), glm::vec3(shift, 0.0f, 1e-4f));
	m_renderer.set_shape_set_transform(m_cursor_shape_set, shifted_pose * m_cursor_transform);
}

void ProgressBar::init_cursor(const glm::vec2& cursor_size)
{
	const float eps = 1e-4f;

	const std::vector<Vertex> svertices = {
		Vertex(glm::vec3(-0.5f * cursor_size.x, -0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.5f * cursor_size.x,  0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f * cursor_size.x,  0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.5f * cursor_size.x, -0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> sindices = {
		0, 1, 2,
		0, 3, 1,
	};

	m_cursor_shape_set = m_renderer.create_shape_set();
	m_renderer.add_shape(m_cursor_shape_set, svertices, sindices);
	std::vector<size_t> cursor_textures;

	if (m_cursor_uses_texture)
	{
		cursor_textures.push_back(m_cursor_texture);
	}

	const size_t shader_id = Application::instance().shader().id();
	m_cursor_renderable = m_renderer.create_shape_set_renderable(m_cursor_shape_set,
	                                                             cursor_textures,
	                                                             shader_id,
	                                                             false);
}

void ProgressBar::draw(void) const
{
	ShaderSet& shader = Application::instance().shader();
	shader.set_uniform("background", m_background_uses_texture);
	m_renderer.draw(m_background_renderable);
	shader.set_uniform("background", m_cursor_uses_texture);
	m_renderer.draw(m_cursor_renderable);
	shader.set_uniform("background", false);
}

void ProgressBar::update_state(void)
{
	set_cursor_position();
}

bool ProgressBar::update_on_interaction(const intersection_t isec, const OpenVRInterface::input_state_t& input)
{
	if (input.trigger.button.released && isec.hit)
	{
		const float step = m_progress_max * isec.local.x - m_progress_pos;
		Application::instance().player().jump(step);
	}

	return input.trigger.button.released && isec.hit;
}
