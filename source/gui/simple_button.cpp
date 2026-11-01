// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "simple_button.h"
#include "application.h"
#include "util/file_system.h"
#include <stdexcept>

static const glm::vec4 button_color(0.0f, 0.0f, 0.0f, 0.0f);

const glm::vec2 SimpleButton::m_button_size(0.5f, 0.5f);    // size in scene coordinate space

SimpleButton::SimpleButton(const action_t action, const std::string& image_name) :
	Panel(action),
	m_renderer(RenderManager::instance()),
	m_renderable_id(0),
	m_shape_set_id(0),
	m_graphic_transform(1.0f),
	m_graphic_uses_texture(true)
{
	init_shape(m_button_size, button_color);

	const std::string ext = FileSystem().extension(image_name);

	if (FileSystem().is_image(ext))
	{
		init_texture(image_name);
		m_shape_set_id = Panel::shape_set_id();
		m_renderable_id = m_renderer.create_shape_set_renderable(m_shape_set_id,
		                                                         std::vector<size_t>(1, texture_id()),
		                                                         Application::instance().shader().id(),
		                                                         false);
	}
	else if (FileSystem().is_vector(ext))
	{
		m_shape_set_id = m_renderer.load_svg(image_name);
		m_graphic_uses_texture = false;
		m_graphic_transform = glm::scale(glm::mat4(1.0f), glm::vec3(m_button_size.x * 0.5f,
		                                                            m_button_size.y * 0.5f,
		                                                            1.0f));
		m_renderable_id = m_renderer.create_shape_set_renderable(m_shape_set_id,
		                                                         std::vector<size_t>(),
		                                                         Application::instance().shader().id(),
		                                                         false);
	}
	else
	{
		throw std::runtime_error("unsupported image format: " + ext);
	}
}

SimpleButton::~SimpleButton(void)
{
	m_renderer.delete_renderable(m_renderable_id);
}

void SimpleButton::set_graphic(const size_t shape_set_id,
                               const bool use_texture,
                               const glm::mat4& local_transform)
{
	std::vector<size_t> textures;

	if (use_texture)
	{
		textures.push_back(texture_id());
	}
	const RenderManager::RenderableId new_renderable = m_renderer.create_shape_set_renderable(shape_set_id,
	                                                                                          textures,
	                                                                                          Application::instance().shader().id(),
	                                                                                          false);
	m_renderer.delete_renderable(m_renderable_id);
	m_renderable_id = new_renderable;
	m_shape_set_id = shape_set_id;
	m_graphic_transform = local_transform;
	m_graphic_uses_texture = use_texture;
	m_renderer.set_shape_set_transform(m_shape_set_id, pose() * m_graphic_transform);
}

void SimpleButton::set_transform(const glm::mat4& pose)
{
	Panel::set_transform(pose);
	set_graphic_pose(pose);
}

void SimpleButton::set_graphic_pose(const glm::mat4& pose)
{
	m_renderer.set_shape_set_transform(m_shape_set_id, pose * m_graphic_transform);
}

bool SimpleButton::update_on_interaction(const Panel::intersection_t isec, const OpenVRInterface::input_state_t& input)
{
	return input.trigger.button.released && isec.hit;
}

void SimpleButton::draw(void) const
{
	Application::instance().shader().set_uniform("background", m_graphic_uses_texture);
	m_renderer.draw(m_renderable_id);
	Application::instance().shader().set_uniform("background", false);
}
