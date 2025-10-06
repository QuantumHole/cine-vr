// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "button.h"
#include "util/id.h"

Button::Button(void) :
	m_id(ID::unique_id()),
	m_action(BUTTON_NONE),
	m_active(true),
	m_shape(),
	m_tex(),
	m_size(0.0f, 0.0f),
	m_pose(1.0f)
{
}

void Button::init(const float size, const button_action_t action)
{
	m_size = glm::vec2(size, size);
	m_action = action;
	// positions: rectangle in XY plane centered at 0, z=0
	const std::vector<Vertex> vertices = {
		Vertex(glm::vec3(-0.5f * m_size.x, -0.5f * m_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3(0.5f * m_size.x, 0.5f * m_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f * m_size.x, 0.5f * m_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3(0.5f * m_size.x, -0.5f * m_size.y, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> indices = {
		0, 1, 2,
		0, 3, 1,
	};

	m_shape.init_vertices(vertices, indices, GL_TRIANGLES);

	std::map<Button::button_action_t, std::string> images = {
		{Button::BUTTON_FILE_DELETE, "images/delete.png"},
		{Button::BUTTON_FILE_OPEN, "images/open.png"},
		{Button::BUTTON_PLAY_BACKWARD, "images/backward.png"},
		{Button::BUTTON_PLAY_FORWARD, "images/forward.png"},
		{Button::BUTTON_PLAY_NEXT, "images/next.png"},
		{Button::BUTTON_PLAY_PAUSE, "images/pause.png"},
		{Button::BUTTON_PLAY_PLAY, "images/play.png"},
		{Button::BUTTON_PLAY_PREVIOUS, "images/previous.png"},
		{Button::BUTTON_POWER, "images/power.png"},
		{Button::BUTTON_PROJECT_CUBE, "images/cube-mono.png"},
		{Button::BUTTON_PROJECT_CYLINDER, "images/cylinder.png"},
		{Button::BUTTON_PROJECT_FISHEYE, "images/fisheye.png"},
		{Button::BUTTON_PROJECT_FLAT, "images/flat.png"},
		{Button::BUTTON_PROJECT_SPHERE, "images/sphere.png"},
		{Button::BUTTON_TILE_CUBE_MONO, "images/cube-mono.png"},
		{Button::BUTTON_TILE_CUBE_STEREO, "images/cube-stereo.png"},
		{Button::BUTTON_TILE_LEFT_RIGHT, "images/left-right.png"},
		{Button::BUTTON_TILE_MONO, "images/mono.png"},
		{Button::BUTTON_TILE_TOP_BOTTOM, "images/top-bottom.png"}
	};

	std::map<Button::button_action_t, std::string>::const_iterator iter = images.find(action);

	if (iter != images.end())
	{
		m_tex.init_file(iter->second, GL_TEXTURE_2D, 0);
	}
}

void Button::enable(const bool active)
{
	m_active = active;
}

void Button::set_transform(const glm::mat4& pose)
{
	m_pose = pose;
	m_shape.set_transform(pose);
}

Button::intersection_t Button::intersection(const glm::mat4& pose) const
{
	intersection_t isec = {m_id, m_action, false, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)};

	if (!m_active)
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
	isec.local = (glm::vec2(isec.global) + 0.5f * m_size) / m_size;

	isec.hit = ((t > 0) &&   // target plane must be in positive direction
	            (isec.global.x >= -0.5f * m_size.x) && (isec.global.x <= 0.5f * m_size.x) &&
	            (isec.global.y >= -0.5f * m_size.y) && (isec.global.y <= 0.5f * m_size.y));

	// transform back into global coordinate system
	isec.global = glm::vec3(m_pose * glm::vec4(isec.global, 1.0f));

	return isec;
}

void Button::draw(void) const
{
	m_tex.bind();
	m_shape.draw();
	m_tex.unbind();
}
