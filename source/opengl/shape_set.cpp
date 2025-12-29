// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shape_set.h"

ShapeSet::ShapeSet(void) :
	m_shapes()
{
}

ShapeSet::~ShapeSet(void)
{
	for (std::vector<Shape*>::const_iterator iter = m_shapes.begin(); iter != m_shapes.end(); ++iter)
	{
		delete (*iter);
	}
	m_shapes.clear();
}

void ShapeSet::add(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const GLenum type)
{
	Shape* shape = new Shape();

	shape->init_vertices(vertices, indices, type);
	m_shapes.push_back(shape);
}

void ShapeSet::set_transform(const glm::mat4& pose)
{
	for (std::vector<Shape*>::const_iterator iter = m_shapes.begin(); iter != m_shapes.end(); ++iter)
	{
		(*iter)->set_transform(pose);
	}
}

void ShapeSet::draw(void) const
{
	for (std::vector<Shape*>::const_iterator iter = m_shapes.begin(); iter != m_shapes.end(); ++iter)
	{
		(*iter)->draw();
	}
}
