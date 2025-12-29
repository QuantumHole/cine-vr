// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VAO_H
#define VAO_H

#include <openvr/openvr.h>
#include "vbo.h"

class VAO
{
	private:
		GLuint m_id;

	public:
		VAO(void);

		GLuint id(void) const;
		void init(void);
		void link_attrib(const VBO& vbo,
		                 const GLuint layout,
		                 const GLuint num_components,
		                 const GLenum type,
		                 const GLsizei stride,
		                 const void* offset) const;
		void link_openvr_model(const VBO& vbo) const;
		void bind(void) const;
		void unbind(void) const;
		void remove(void);
};

#endif
