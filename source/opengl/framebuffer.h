// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <GL/gl.h>
#include <glm/glm.hpp>

class Framebuffer
{
	private:
		GLuint m_framebuffer_id;
		GLuint m_texture_id;
		GLuint m_depthbuffer_id;
		glm::uvec2 m_size;

	public:
		Framebuffer(void);

		void init(const glm::uvec2& size);
		GLuint id(void) const;
		GLuint texture(void) const;
		const glm::uvec2& size(void) const;
		void bind(const GLenum mode) const;
		void unbind(const GLenum mode) const;
};

#endif
