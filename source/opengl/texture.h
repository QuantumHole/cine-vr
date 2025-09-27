// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/glew.h> // must be included before glcanvas.h
#include <openvr.h>
#include <SDL3/SDL_surface.h>
#include <string>

class Texture
{
	private:
		GLuint m_id;
		GLenum m_type;
		GLuint m_slot;
		GLenum m_format;
		size_t m_width;
		size_t m_height;

		void init(const GLenum tex_type, const GLuint slot, const GLint filter);

	public:
		Texture(void);

		GLuint id(void) const;
		GLuint slot(void) const;

		void init_file(const std::string& file_name, const GLenum tex_type, const GLuint slot, const GLint filter = GL_NEAREST);
		void init_sdl(const SDL_Surface* surface, const GLenum tex_type, const GLuint slot, const GLint filter = GL_NEAREST);
		void init_dim(const size_t width, const size_t height, const GLenum tex_type, const GLuint slot, const GLint filter = GL_NEAREST);
		void init_openvr_model(const std::string& name, const GLenum tex_type, const GLuint slot, const GLint filter = GL_NEAREST);
		void bind(void) const;
		void unbind(void) const;
		void remove(void);
};

#endif
