// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VERTEX_H
#define VERTEX_H

#include <GL/gl.h>
#include <glm/glm.hpp>

class Vertex
{
	private:
		glm::vec3 m_position;
		glm::vec3 m_normal;
		glm::vec4 m_color;
		glm::vec2 m_tex_uv;

	public:
		Vertex(void);
		explicit Vertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec4& color, const glm::vec2& tex_uv);
		Vertex(const Vertex&);
		Vertex& operator=(const Vertex&);

		const GLfloat* data(void) const;
		const glm::vec3& position(void) const;
		const glm::vec3& normal(void) const;
		const glm::vec4& color(void) const;
		const glm::vec2& tex(void) const;
};

#endif
