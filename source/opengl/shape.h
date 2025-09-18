// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SHAPE_H
#define SHAPE_H

#include "vbo.h"
#include "ebo.h"
#include "vao.h"
#include <map>

class Shape
{
	private:
		VAO m_vao;
		VBO m_vbo;
		EBO m_ebo;
		VBO m_vbo_instances;
		size_t m_instances;
		GLenum m_shape_type;

		void init_buffers(void);

	public:
		explicit Shape(void);
		Shape(const Shape&);
		Shape& operator=(const Shape&);
		virtual ~Shape(void);

		void set_instances(const size_t instances);
		size_t instances(void) const;
		void set_transform(const glm::mat4& transform, const size_t instance = 0) const;

		void init_vertices(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const GLenum type = GL_TRIANGLES);
		void init_openvr_model(const std::string& name);

		void draw(void) const;
		void remove(void);
};

#endif
