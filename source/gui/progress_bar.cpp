// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "progress_bar.h"
#include "main.h"
#include "util/file_system.h"
#include <iostream>
#include <stdexcept>

static const float tex2shape = 0.01f;

ProgressBar::ProgressBar(const action_t action, const std::string& image, const float max, const float pos) :
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
	m_background_transform(1.0f),
	m_background_uses_texture(false)
{
	const std::string ext = FileSystem().extension(image);
	if (FileSystem().is_image(ext))
	{
		init_texture(image);
		m_progress_size = tex2shape * glm::vec2(texture().size());
		m_background_uses_texture = true;
	}
	else if (FileSystem().is_vector(ext))
	{
		m_background_shape_set = m_renderer.load_svg(image);
		m_progress_size = tex2shape * glm::vec2(605.0f, 29.0f);
		const float svg_scale = 0.5f * m_progress_size.x;
		m_background_transform = glm::scale(glm::mat4(1.0f), glm::vec3(svg_scale, svg_scale, 1.0f));
		m_background_renderable = m_renderer.create_shape_set_renderable(m_background_shape_set,
		                                                                    std::vector<size_t>(),
		                                                                    shader().id(),
		                                                                    false);
	}
	else
	{
		throw std::runtime_error("unsupported image format: " + ext);
	}

	if ((m_progress_size.x <= 0.0f) || (m_progress_size.y <= 0.0f))
	{
		throw std::runtime_error("invalid progress bar dimensions");
	}
	const glm::vec4 color(0.0f);
	init_shape(m_progress_size, color);
	if (m_background_uses_texture)
	{
		m_background_shape_set = m_renderer.create_shape_set();
		m_renderer.add_shape(m_background_shape_set, shape_vertices(), shape_indices());
	}
	std::vector<size_t> background_textures;
	if (m_background_uses_texture)
	{
		background_textures.push_back(texture_id());
	}
	m_background_renderable = m_renderer.create_shape_set_renderable(m_background_shape_set,
	                                                                    background_textures,
	                                                                    shader().id(),
	                                                                    false);
	const size_t cursor_texture = m_renderer.create_texture();
	m_cursor_texture = cursor_texture;
	Texture& cursor_image = m_renderer.texture(m_cursor_texture);
	cursor_image.init_image_file("images/progress-cursor.png", 0);
	const glm::vec2 cursor_size = tex2shape * glm::vec2(cursor_image.size());
	init_cursor(cursor_size);
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
	m_progress_pos = player().playtime();
	const float frac = glm::clamp(m_progress_pos / m_progress_max, 0.0f, 1.0f);
	const float shift = (frac - 0.5f) * m_progress_size.x;

	glm::mat4 shifted_pose = glm::translate(Panel::pose(), glm::vec3(shift, 0.0f, 0.0f));
	m_renderer.set_shape_set_transform(m_cursor_shape_set, shifted_pose);
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
	m_cursor_renderable = m_renderer.create_shape_set_renderable(m_cursor_shape_set,
	                                                              std::vector<size_t>(1, m_cursor_texture),
	                                                              shader().id(),
	                                                              false);
}

void ProgressBar::draw(void) const
{
	shader().set_uniform("background", m_background_uses_texture);
	m_renderer.draw(m_background_renderable);
	shader().set_uniform("background", true);
	m_renderer.draw(m_cursor_renderable);
	shader().set_uniform("background", false);
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
		player().jump(step);
	}

	return input.trigger.button.released && isec.hit;
}
