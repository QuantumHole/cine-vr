// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "texture.h"
#include "util/image_data.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>

Texture::Texture(void) :
	m_id(0),
	m_type(),
	m_slot(0),
	m_format(GL_RGB),
	m_width(0),
	m_height(0)
{
}

GLuint Texture::id(void) const
{
	return m_id;
}

void Texture::init(const GLenum tex_type, const GLenum slot, const GLint filter)
{
	m_type = tex_type;
	m_slot = slot;

	if (!m_id)
	{
		glGenTextures(1, &m_id);
	}

	if (m_id == 0)
	{
		throw std::runtime_error("texture not initialized");
	}

	glActiveTexture(GL_TEXTURE0 + m_slot);
	glBindTexture(tex_type, m_id);
	glTexParameteri(tex_type, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(tex_type, GL_TEXTURE_MAG_FILTER, filter);
	// glTexParameteri(tex_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	// glTexParameteri(tex_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// glTexParameteri(tex_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(tex_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(tex_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(tex_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(tex_type, GL_TEXTURE_MAX_LEVEL, 0);

	// std::vector<float> flatColor = {1.0f, 1.0f, 1.0f, 1.0f};
	// glTexParameterfv(tex_type, GL_TEXTURE_BORDER_COLOR, flatColor.data());

	float fLargest = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
}

void Texture::init_file(const std::string& file_name, const GLenum tex_type, const GLuint slot, const GLint filter)
{
	ImageFile m_image(file_name);

	m_format = (m_image.has_alpha_channel() ? GL_RGBA : GL_RGB);
	init(tex_type, slot, filter);
	glTexImage2D(tex_type, 0, GL_RGBA, static_cast<GLsizei>(m_image.width()), static_cast<GLsizei>(m_image.height()), 0, m_format, GL_UNSIGNED_BYTE, m_image.pixels().data());
}

void Texture::init_dim(const size_t width, const size_t height, const GLenum tex_type, const GLuint slot, const GLint filter)
{
	if (m_width || m_height)
	{
		remove();
	}

	m_width = width;
	m_height = height;
	init(tex_type, slot, filter);

	if ((m_width > 0) && (m_height > 0))
	{
		// glTexImage2D(tex_type, 0, GL_RGB32F, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexStorage2D(tex_type, 1, GL_RGBA32F, static_cast<GLuint>(m_width), static_cast<GLuint>(m_height));
		glBindImageTexture(m_slot, m_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}
}

void Texture::init_sdl(const SDL_Surface* surface, const GLenum tex_type, const GLuint slot, const GLint filter)
{
	init(tex_type, slot, filter);
	glTexImage2D(tex_type, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surface->pixels);
}

void Texture::init_openvr_model(const std::string& name, const GLenum tex_type, const GLuint slot, const GLint filter)
{
	vr::RenderModel_t* model;
	vr::RenderModel_TextureMap_t* texture;
	vr::EVRRenderModelError error;

	while (true)
	{
		error = vr::VRRenderModels()->LoadRenderModel_Async(name.c_str(), &model);

		if (error != vr::VRRenderModelError_Loading)
		{
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if (error != vr::VRRenderModelError_None)
	{
		std::stringstream s;
		s << "Unable to load render model " << name << " - " << vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error);
		throw std::runtime_error(s.str());
	}

	while (true)
	{
		error = vr::VRRenderModels()->LoadTexture_Async(model->diffuseTextureId, &texture);

		if (error != vr::VRRenderModelError_Loading)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if (error != vr::VRRenderModelError_None)
	{
		std::stringstream s;
		s << "Unable to load render texture id:" << model->diffuseTextureId << " for render model " << name;
		vr::VRRenderModels()->FreeRenderModel(model);
		throw std::runtime_error(s.str());
	}

	init(tex_type, slot, filter);
	glTexImage2D(tex_type, 0, GL_RGBA, texture->unWidth, texture->unHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->rubTextureMapData);

	vr::VRRenderModels()->FreeRenderModel(model);
	vr::VRRenderModels()->FreeTexture(texture);
}

GLuint Texture::slot(void) const
{
	return m_slot;
}

void Texture::bind(void) const
{
	if ((m_width > 0) && (m_height > 0))
	{
		glActiveTexture(GL_TEXTURE0 + m_slot);
		glBindTextureUnit(0, m_id);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0 + m_slot);
		glBindTexture(m_type, m_id);
	}
}

void Texture::unbind(void) const
{
	if (m_id)
	{
		glBindTexture(m_type, 0);
	}
}

void Texture::remove(void)
{
	if (m_id)
	{
		glDeleteTextures(1, &m_id);
		m_id = 0;
	}
}
