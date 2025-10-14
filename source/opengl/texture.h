// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/glew.h> // must be included before glcanvas.h
#include <openvr.h>
#include <SDL3/SDL_surface.h>
#include <string>
#include <glm/glm.hpp>

class Texture
{
	private:
		GLuint m_id;
		GLenum m_type;
		GLuint m_slot;
		GLenum m_format;
		glm::uvec2 m_size;

		void init(const GLenum tex_type, const GLuint slot, const GLint filter);

	public:
		Texture(void);

		GLuint id(void) const;
		GLuint slot(void) const;

		glm::uvec2 init_file(const std::string& file_name, const GLenum tex_type, const GLuint slot, const GLint filter = GL_NEAREST);
		void init_sdl(const SDL_Surface* surface, const GLenum tex_type, const GLuint slot, const GLint filter = GL_NEAREST);
		void init_dim(const glm::uvec2 size, const GLenum tex_type, const GLuint slot, const GLint filter = GL_NEAREST);
		void init_openvr_model(const std::string& name, const GLenum tex_type, const GLuint slot, const GLint filter = GL_NEAREST);
		void bind(void) const;
		void unbind(void) const;
		void remove(void);
};

#endif
