// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define GL_GLEXT_PROTOTYPES

#include "vao.h"

VAO::VAO(void) :
	m_id(0)
{
}

GLuint VAO::id(void) const
{
	return m_id;
}

void VAO::init(void)
{
	glGenVertexArrays(1, &m_id);
	glBindVertexArray(m_id);
	// glCreateVertexArrays(1, &iVAO);
}

void VAO::link_attrib(const VBO& vbo, const GLuint layout, const GLuint num_components, const GLenum type, const GLsizei stride, const void* offset) const
{
	vbo.bind();
	glVertexAttribPointer(layout, num_components, type, GL_FALSE, stride, offset);
	glEnableVertexAttribArray(layout);
}

void VAO::link_openvr_model(const VBO& vbo) const
{
	vbo.bind();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
	link_attrib(vbo, 0, 3, GL_FLOAT, sizeof(vr::RenderModel_Vertex_t), reinterpret_cast<void*>(offsetof(vr::RenderModel_Vertex_t, vPosition)));
#pragma GCC diagnostic pop
	link_attrib(vbo, 1, 3, GL_FLOAT, sizeof(vr::RenderModel_Vertex_t), reinterpret_cast<void*>(offsetof(vr::RenderModel_Vertex_t, vNormal)));
	link_attrib(vbo, 3, 2, GL_FLOAT, sizeof(vr::RenderModel_Vertex_t), reinterpret_cast<void*>(offsetof(vr::RenderModel_Vertex_t, rfTextureCoord)));
}

void VAO::bind(void) const
{
	glBindVertexArray(m_id);
}

void VAO::unbind(void) const
{
	glBindVertexArray(0);
}

void VAO::remove(void)
{
	if (m_id)
	{
		glDeleteVertexArrays(1, &m_id);
		m_id = 0;
	}
}
