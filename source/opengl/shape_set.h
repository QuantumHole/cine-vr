// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SHAPE_SET_H
#define SHAPE_SET_H

#include <vector>
#include <glm/glm.hpp>
#include "opengl/shape.h"

class ShapeSet
{
	private:
		// order of drawing matters, don't use std::set.
		// Shapes remove themselves from OpenGL when destroyed.
		// So, keep them in memory, handle them via pointers.
		std::vector<Shape*> m_shapes;

	public:
		explicit ShapeSet(void);
		virtual ~ShapeSet(void);
		void add(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const GLenum type = GL_TRIANGLES);
		void set_transform(const glm::mat4& pose);
		void draw(void) const;
};

#endif
