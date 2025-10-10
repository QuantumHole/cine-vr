// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "button.h"
#include "util/id.h"
#include "main.h"

static const float slide_height = 10.0f;

const glm::vec2 Button::m_size(0.5f, 0.5f);    // size in scene coordinate space

Button::Button(const button_action_t action) :
	m_action(action),
	m_toggleable(false),
	m_active(true),
	m_slideable(false),
	m_slide_min(0.0f),
	m_slide_max(0.0f),
	m_slide_pos(0.0f),
	m_slide_last(0.0f),
	m_shape(),
	m_slidebar(),
	m_tex(),
	m_pose(1.0f)
{
	init();
}

Button::Button(const button_action_t action, const bool value) :
	m_action(action),
	m_toggleable(true),
	m_active(value),
	m_slideable(false),
	m_slide_min(0.0f),
	m_slide_max(0.0f),
	m_slide_pos(0.0f),
	m_slide_last(0.0f),
	m_shape(),
	m_slidebar(),
	m_tex(),
	m_pose(1.0f)
{
	init();
}

Button::Button(const button_action_t action, const float min, const float max, const float value) :
	m_action(action),
	m_toggleable(false),
	m_active(false),
	m_slideable(true),
	m_slide_min(min),
	m_slide_max(max),
	m_slide_pos(value),
	m_slide_last(value),
	m_shape(),
	m_slidebar(),
	m_tex(),
	m_pose(1.0f)
{
	init();
}

Button::~Button(void)
{
	m_shape.remove();

	if (m_slideable)
	{
		m_slidebar.remove();
	}
}

void Button::init(void)
{
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
		{Button::BUTTON_TILE_TOP_BOTTOM, "images/top-bottom.png"},
		{Button::BUTTON_FLAG_MONO, "images/force-mono.png"},
		{Button::BUTTON_FLAG_STRETCH, "images/stretch.png"},
		{Button::BUTTON_FLAG_SWITCH_EYES, "images/switch-eyes.png"},
		{Button::BUTTON_PARAM_ANGLE, "images/angle.png"},
		{Button::BUTTON_PARAM_ZOOM, "images/zoom.png"}
	};

	std::map<Button::button_action_t, std::string>::const_iterator iter = images.find(m_action);

	if (iter != images.end())
	{
		m_tex.init_file(iter->second, GL_TEXTURE_2D, 0);
	}

	std::set<Button::button_action_t> toggleables = {
		BUTTON_FLAG_MONO,
		BUTTON_FLAG_STRETCH,
		BUTTON_FLAG_SWITCH_EYES
	};

	std::set<Button::button_action_t> slideables = {
		BUTTON_PARAM_ANGLE,
		BUTTON_PARAM_ZOOM
	};

	if (slideables.find(m_action) != slideables.end())
	{
		const float eps = 1e-4f;
		const float y0 = 0.0f;
		const float y1 = slide_height * m_size.y;

		const std::vector<Vertex> svertices = {
			Vertex(glm::vec3(-0.4f * m_size.x, y0, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)),
			Vertex(glm::vec3(0.4f * m_size.x, y1, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
			Vertex(glm::vec3(-0.4f * m_size.x, y1, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
			Vertex(glm::vec3(0.4f * m_size.x, y0, -eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
		};

		const std::vector<GLuint> sindices = {
			0, 1, 2,
			0, 3, 1,
		};

		m_slidebar.init_vertices(svertices, sindices, GL_TRIANGLES);
	}
}

bool Button::toggleable(void) const
{
	return m_toggleable;
}

void Button::enable(const bool active)
{
	if (m_slideable && !m_active && active)
	{
		m_slide_last = m_slide_pos;

		const float frac = (m_slide_pos - m_slide_min) / (m_slide_max - m_slide_min);
		const float y0 = -slide_height * m_size.y * frac;

		// move selection strip to current position
		glm::mat4 shifted_pose = glm::translate(m_pose, glm::vec3(0.0f, y0, 0.0f));
		m_slidebar.set_transform(shifted_pose);
	}
	else if (m_slideable && m_active && !active)
	{
		m_shape.set_transform(m_pose);
	}
	m_active = active;
}

bool Button::active(void) const
{
	return m_active;
}

bool Button::slideable(void) const
{
	return m_slideable;
}

float Button::slide_value(void) const
{
	return m_slide_pos;
}

void Button::update_slide_value(const float pos)
{
	if (m_slideable && m_active)
	{
		m_slide_pos = m_slide_min + pos * (m_slide_max - m_slide_min);
	}
}

void Button::set_transform(const glm::mat4& pose)
{
	m_pose = pose;
	m_shape.set_transform(pose);
}

Button::intersection_t Button::intersection(const glm::mat4& pose) const
{
	intersection_t isec = {m_action, false, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)};

	if (m_action == BUTTON_NONE)
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

	if (m_slideable && m_active)
	{
		// restrict y coordinate of shown icon from last slide position
		const float frac = (m_slide_last - m_slide_min) / (m_slide_max - m_slide_min);
		const float y0 = -slide_height * m_size.y * frac;
		const float y1 = slide_height * m_size.y * (1.0f - frac);
		const float global_y = std::min(std::max(isec.global.y, y0), y1);

		isec.local = (glm::vec2(isec.global.x, global_y) - y0) / (y1 - y0);

		// move button icon to cursor position
		glm::mat4 shifted_pose = glm::translate(m_pose, glm::vec3(0.0f, global_y, 0.0f));
		m_shape.set_transform(shifted_pose);
	}
	else
	{
		// button coordinates [0; 1]
		isec.local = (glm::vec2(isec.global) + 0.5f * m_size) / m_size;
	}

	isec.hit = ((t > 0) &&   // target plane must be in positive direction
	            (isec.global.x >= -0.5f * m_size.x) && (isec.global.x <= 0.5f * m_size.x) &&
	            (isec.global.y >= -0.5f * m_size.y) && (isec.global.y <= 0.5f * m_size.y));

	// transform back into global coordinate system
	isec.global = glm::vec3(m_pose * glm::vec4(isec.global, 1.0f));

	return isec;
}

void Button::draw(void) const
{
	if (m_toggleable && !m_active)
	{
		shader().set_uniform("greyscale", true);
	}

	m_tex.bind();
	m_shape.draw();
	m_tex.unbind();

	if (m_toggleable && !m_active)
	{
		shader().set_uniform("greyscale", false);
	}

	if (m_slideable && m_active)
	{
		m_slidebar.draw();
	}
}
