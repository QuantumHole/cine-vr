// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ebo.h"

EBO::EBO(void) :
	m_id(0),
	m_num_indices(0),
	m_index_format(GL_UNSIGNED_INT)
{
}

GLuint EBO::id(void) const
{
	return m_id;
}

GLuint EBO::num_indices(void) const
{
	return m_num_indices;
}

GLenum EBO::index_format(void) const
{
	return m_index_format;
}

void EBO::init(const std::vector<GLuint>& indices)
{
	glGenBuffers(1, &m_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

	m_num_indices = static_cast<GLuint>(indices.size());
	m_index_format = GL_UNSIGNED_INT;
}

void EBO::init(const vr::RenderModel_t& model)
{
	glGenBuffers(1, &m_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * model.unTriangleCount * 3, model.rIndexData, GL_STATIC_DRAW);

	m_num_indices = model.unTriangleCount * 3;
	m_index_format = GL_UNSIGNED_SHORT;
}

void EBO::bind(void) const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
}

void EBO::unbind(void) const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::remove(void)
{
	if (m_id)
	{
		glDeleteBuffers(1, &m_id);
		m_id = 0;
	}
}
