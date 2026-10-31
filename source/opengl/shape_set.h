// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SHAPE_SET_H
#define SHAPE_SET_H

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "opengl/shape.h"

class ShapeSet
{
	private:
		// order of drawing matters, don't use std::set.
		std::vector<std::unique_ptr<Shape> > m_shapes;

	public:
		explicit ShapeSet(void);
		virtual ~ShapeSet(void);

		/** add a Shape to this set.
		 * @param vertices vertices for the VBO.
		 * @param indices indices for the EBO.
		 * @param type type of elements. Supported values are GL_TRIANGLES, GL_LINES, GL_POINTS.
		 * @param instances number of instances of this shape. Defaults to 1.
		 * @return index of the new shape in this shape set.
		 */
		size_t add(const std::vector<Vertex>& vertices,
		           const std::vector<GLuint>& indices,
		           const GLenum type = GL_TRIANGLES,
		           const size_t instances = 1);
		void clear(void);
		void set_transform(const glm::mat4& pose);
		void set_transform(const size_t index, const glm::mat4& pose, const size_t instance = 0);
		void draw(void) const;
};

#endif
