// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vertex.h"

Vertex::Vertex(void) :
	m_position(),
	m_normal(),
	m_color(),
	m_tex_uv()
{
}

Vertex::Vertex(const glm::vec3& position,
               const glm::vec3& normal,
               const glm::vec3& color,
               const glm::vec2& tex_uv) :
	m_position(position),
	m_normal(normal),
	m_color(color),
	m_tex_uv(tex_uv)
{
}

Vertex::Vertex(const Vertex& v) :
	m_position(v.m_position),
	m_normal(v.m_normal),
	m_color(v.m_color),
	m_tex_uv(v.m_tex_uv)
{
}

Vertex& Vertex::operator=(const Vertex& v)
{
	m_position = v.m_position;
	m_normal = v.m_normal;
	m_color = v.m_color;
	m_tex_uv = v.m_tex_uv;
	return *this;
}

const GLfloat* Vertex::data(void) const
{
	return &m_position.x;
}

const glm::vec3& Vertex::position(void) const
{
	return m_position;
}

const glm::vec3& Vertex::normal(void) const
{
	return m_normal;
}

const glm::vec3& Vertex::color(void) const
{
	return m_color;
}

const glm::vec2& Vertex::tex(void) const
{
	return m_tex_uv;
}
