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
		GLuint m_slot;
		GLenum m_format;
		glm::uvec2 m_size;

		void init(const GLuint slot);

	public:
		Texture(void);

		GLuint id(void) const;
		GLuint slot(void) const;

		glm::uvec2 init_image_file(const std::string& file_name, const GLuint slot);
		void init_sdl(const SDL_Surface* surface, const GLuint slot);
		void init_dim(const glm::uvec2 size, const GLuint slot);
		void init_openvr_model(const std::string& name, const GLuint slot);
		void bind(void) const;
		void unbind(void) const;
		void remove(void);
};

#endif
