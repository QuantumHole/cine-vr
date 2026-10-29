// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
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

size_t ShapeSet::add(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const GLenum type, const size_t instances)
{
	Shape* shape = new Shape();

	shape->init_vertices(vertices, indices, type);
	shape->set_instances(instances);
	m_shapes.push_back(shape);
	return m_shapes.size() - 1;
}

void ShapeSet::set_transform(const glm::mat4& pose)
{
	for (std::vector<Shape*>::const_iterator iter = m_shapes.begin(); iter != m_shapes.end(); ++iter)
	{
		for (size_t i = 0; i < (*iter)->instances(); i++)
		{
			(*iter)->set_transform(pose, i);
		}
	}
}

void ShapeSet::set_transform(const size_t index, const glm::mat4& pose, const size_t instance)
{
	m_shapes.at(index)->set_transform(pose, instance);
}

void ShapeSet::draw(void) const
{
	for (std::vector<Shape*>::const_iterator iter = m_shapes.begin(); iter != m_shapes.end(); ++iter)
	{
		(*iter)->draw();
	}
}
