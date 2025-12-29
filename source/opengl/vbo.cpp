// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define GL_GLEXT_PROTOTYPES

#include "vbo.h"

VBO::VBO(void) :
	m_id(0),
	m_num_vertices(0)
{
}

GLuint VBO::id(void) const
{
	return m_id;
}

GLuint VBO::num_vertices(void) const
{
	return m_num_vertices;
}

void VBO::init(void)
{
	glGenBuffers(1, &m_id);
}

void VBO::load_data(const std::vector<Vertex>& vertices)
{
	bind();
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
	// glNamedBufferData(VBO, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	m_num_vertices = static_cast<GLuint>(vertices.size());
}

void VBO::load_data(const vr::RenderModel_t& model)
{
	bind();
	glBufferData(GL_ARRAY_BUFFER, sizeof(vr::RenderModel_Vertex_t) * model.unVertexCount, model.rVertexData, GL_STATIC_DRAW);

	m_num_vertices = model.unVertexCount;
}

void VBO::load_data(const std::vector<glm::mat4>& transforms) const
{
	bind();
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(transforms.size() * sizeof(glm::mat4)), transforms.data(), GL_STATIC_DRAW);
}

void VBO::bind(void) const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_id);
}

void VBO::unbind(void) const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::remove(void)
{
	if (m_id)
	{
		glDeleteBuffers(1, &m_id);
		m_id = 0;
	}
}
