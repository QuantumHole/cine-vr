// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EBO_H
#define EBO_H

#include <GL/gl.h>
#include <openvr/openvr.h>
#include <vector>

class EBO
{
	private:
		GLuint m_id;
		GLuint m_num_indices;
		GLenum m_index_format;

	public:
		EBO(void);

		GLuint id(void) const;
		GLuint num_indices(void) const;
		GLenum index_format(void) const;

		void init(const std::vector<GLuint>& vertices);
		void init(const vr::RenderModel_t& model);
		void bind(void) const;
		void unbind(void) const;
		void remove();
};

#endif
