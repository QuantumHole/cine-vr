// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <unistd.h>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include "render_model.h"

RenderModel::RenderModel(void) :
	m_shape(),
	m_tex(),
	m_position(0.0, 0.0, 0.0),
	m_rotation(0.0, 0.0, 0.0, 0.0)
{
}

RenderModel::RenderModel(const RenderModel& rm) :
	m_shape(rm.m_shape),
	m_tex(rm.m_tex),
	m_position(rm.m_position),
	m_rotation(rm.m_rotation)
{
}

RenderModel& RenderModel::operator=(const RenderModel& rm)
{
	m_shape = rm.m_shape;
	m_tex = rm.m_tex;
	m_position = rm.m_position;
	m_rotation = rm.m_rotation;
	return *this;
}

RenderModel::~RenderModel(void)
{
	cleanup();
}

void RenderModel::init_openvr_model(const std::string& name)
{
	m_shape.init_openvr_model(name);
	m_tex.init_openvr_model(name, 0);
	m_tex.unbind();
}

void RenderModel::cleanup(void)
{
	m_shape.remove();
	m_tex.remove();
}

void RenderModel::draw(void) const
{
	const glm::mat4 matrix = glm::translate(glm::mat4(1.0), m_position) * mat4_cast(m_rotation);

	m_shape.set_transform(matrix);

	m_tex.bind();
	m_shape.draw();
	m_tex.unbind();
}

void RenderModel::set_transform(const glm::mat4& matrix)
{
	m_position = matrix * glm::vec4(0, 0, 0, 1);
	m_rotation = glm::quat_cast(matrix);
}

void RenderModel::set_transform(const glm::vec3& position, const glm::quat& rotation)
{
	m_position = position;
	m_rotation = rotation;
}
