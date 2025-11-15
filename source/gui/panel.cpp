// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "panel.h"
#include "font_renderer.h"
#include "main.h"

#include <stdexcept>
#include <algorithm>

Panel::Panel(const action_t action) :
	m_shape(),
	m_texture(),
	m_pose(1.0f),
	m_shape_size(0.0f, 0.0f),
	m_action(action)
{
}

Panel::~Panel(void)
{
	m_shape.remove();
	m_texture.remove();
}

void Panel::init_shape(const glm::vec2& shape_size, const glm::vec4& color)
{
	// reinitialize
	m_shape.remove();

	m_shape_size = shape_size;

	// positions: rectangle in XY plane centered at 0, z=0
	const std::vector<Vertex> vertices = {
		Vertex(glm::vec3(-0.5f * m_shape_size.x, -0.5f * m_shape_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), color, glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3(0.5f * m_shape_size.x, 0.5f * m_shape_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), color, glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f * m_shape_size.x, 0.5f * m_shape_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), color, glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3(0.5f * m_shape_size.x, -0.5f * m_shape_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), color, glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> indices = {
		0, 1, 2,
		0, 3, 1,
	};

	m_shape.init_vertices(vertices, indices, GL_TRIANGLES);
}

void Panel::init_texture(const std::string& image_name)
{
	m_texture.init_image_file(image_name, 0);
}

void Panel::init_texture(const glm::uvec2& tex_size)
{
	m_texture.init_dim(tex_size, 0);
}

void Panel::init_area(const glm::vec2& shape_size, const glm::vec4& color, const glm::uvec2& tex_size)
{
	init_shape(shape_size, color);
	init_texture(tex_size);
}

void Panel::set_transform(const glm::mat4& pose)
{
	m_pose = pose;
	m_shape.set_transform(pose);
}

const glm::mat4& Panel::pose(void) const
{
	return m_pose;
}

const Shape& Panel::shape(void) const
{
	return m_shape;
}

const Texture& Panel::texture(void) const
{
	return m_texture;
}

const glm::uvec2& Panel::tex_size(void) const
{
	return m_texture.size();
}

void Panel::clear(void)
{
	const glm::uvec2& size = m_texture.size();
	std::vector<uint32_t> image(size.x * size.y, 0);

	m_texture.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0,
	                0,
	                0,
	                static_cast<GLsizei>(size.x),
	                static_cast<GLsizei>(size.y),
	                GL_RGBA, GL_UNSIGNED_BYTE,
	                image.data());
	m_texture.unbind();
}

void Panel::text(const std::string& text, const int32_t x, const int32_t y) const
{
	FontRenderer renderer;

	renderer.load_font("/usr/share/fonts/TTF/OpenSans-Regular.ttf");

	const size_t height = 20;
	std::vector<uint32_t> image;
	renderer.render_text(text, height, image);

	const size_t width = image.size() / height;
	const glm::uvec2& tex_size = m_texture.size();

	/* copy only the visible section of the surface */
	m_texture.bind();
	for (size_t i = 0; i < height; i++)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0,
		                x,
		                y + static_cast<GLint>(i),
		                std::min(static_cast<GLsizei>(width), static_cast<GLsizei>(std::max(0, static_cast<int32_t>(tex_size.x) - x))),
		                1,
		                GL_RGBA, GL_UNSIGNED_BYTE,
		                image.data() + static_cast<ptrdiff_t>(i * width));
	}
	m_texture.unbind();
}

void Panel::draw(void) const
{
	shader().set_uniform("background", true);

	if (m_texture.id())
	{
		m_texture.bind();
	}
	m_shape.draw();

	if (m_texture.id())
	{
		m_texture.unbind();
	}

	shader().set_uniform("background", false);
}

Panel::intersection_t Panel::intersection(const glm::mat4& pose) const
{
	Panel::intersection_t isec = {m_action, false, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)};

	if (m_action == ACTION_NONE)
	{
		return isec;
	}

	// pointing ray: origin = device position, direction = forward -Z in device space transformed to world
	glm::vec3 origin = glm::vec3(pose * glm::vec4(0, 0, 0, 1));
	glm::vec3 direction = glm::normalize(glm::vec3(pose * glm::vec4(0, 0, -1, 0)));

	// Transform ray into rectangle local space.
	// Afterwards, collision can be checked by intersection with the x/y plane at z=0.
	glm::mat4 modelInv = glm::inverse(m_pose);
	glm::vec3 local_origin = glm::vec3(modelInv * glm::vec4(origin, 1.0f));
	glm::vec3 local_direction = glm::normalize(glm::vec3(modelInv * glm::vec4(direction, 0.0f))); // ignore offset/translation with 0.0

	// Ray-plane intersection: plane z=0 in rectangle local space (rect centered at origin, size +/-0.5)
	// rectangle in local coordinates lies on plane z=0

	// ray parallel to z=0 plane.
	if (fabsf(local_direction.z) < std::numeric_limits<float>::epsilon())
	{
		return isec;
	}

	const float t = -local_origin.z / local_direction.z;

	// point of intersection in local coordinates
	// check if it lies within the object boundaries
	isec.global = local_origin + t * local_direction;

	isec.hit = ((t > 0) &&   // target plane must be in positive direction
	            (isec.global.x >= -0.5f * m_shape_size.x) && (isec.global.x <= 0.5f * m_shape_size.x) &&
	            (isec.global.y >= -0.5f * m_shape_size.y) && (isec.global.y <= 0.5f * m_shape_size.y));

	// button coordinates [0; 1]
	isec.local = (glm::vec2(isec.global) + 0.5f * m_shape_size) / m_shape_size;

	// transform back into global coordinate system
	isec.global = glm::vec3(m_pose * glm::vec4(isec.global, 1.0f));

	return isec;
}

bool Panel::update_on_interaction(const intersection_t isec, const OpenVRInterface::input_state_t& input)
{
	return input.trigger.button.released && isec.hit;
}
