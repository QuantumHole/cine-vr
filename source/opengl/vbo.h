// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VBO_H
#define VBO_H

#include <GL/glew.h>
#include <vector>
#include <openvr/openvr.h>

#include "vertex.h"

class VBO
{
	private:
		GLuint m_id;
		GLuint m_num_vertices;

	public:
		VBO(void);

		GLuint id(void) const;
		GLuint num_vertices(void) const;

		void init(void);
		void load_data(const std::vector<Vertex>& vertices);
		void load_data(const vr::RenderModel_t& model);
		void load_data(const std::vector<glm::mat4>& transforms) const;
		void bind(void) const;
		void unbind(void) const;
		void remove();
};

#endif
