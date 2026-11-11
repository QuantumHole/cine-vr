// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define GL_GLEXT_PROTOTYPES

#include "texture.h"
#include "util/image_data.h"
#include <thread>
#include <sstream>

static const GLint internal_format(GL_RGBA);

Texture::Texture(void) :
	m_id(0),
	m_slot(0),
	m_target(GL_TEXTURE_2D),
	m_format(internal_format),
	m_size(0, 0),
	m_depth(0)
{
}

GLuint Texture::id(void) const
{
	return m_id;
}

const glm::uvec2& Texture::size(void) const
{
	return m_size;
}

void Texture::init(const GLenum slot)
{
	m_slot = slot;
	const GLint filter(GL_NEAREST);

	if (!m_id)
	{
		glGenTextures(1, &m_id);
	}

	if (m_id == 0)
	{
		throw std::runtime_error("texture not initialized");
	}

	glActiveTexture(GL_TEXTURE0 + m_slot);
	glBindTexture(m_target, m_id);
	glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, filter);
	// glTexParameteri(tex_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	// glTexParameteri(tex_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// glTexParameteri(tex_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(tex_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (m_target == GL_TEXTURE_3D)
	{
		glTexParameteri(m_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
	glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, GL_RED);
	glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
	glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
	glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

	// std::vector<float> flatColor = {1.0f, 1.0f, 1.0f, 1.0f};
	// glTexParameterfv(tex_type, GL_TEXTURE_BORDER_COLOR, flatColor.data());

	float fLargest = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
}

glm::uvec2 Texture::init_image_file(const std::string& file_name, const GLuint slot)
{
	ImageFile m_image(file_name);

	switch (m_image.bpp())
	{
		case 8:
			m_format = GL_RED;
			break;
		case 16:
			m_format = GL_RG;
			break;
		case 32:
			m_format = GL_RGBA;
			break;
		case 24:
			m_format = GL_RGB;
			break;
		default:
			throw std::runtime_error("unsupported color depth");
	}
	init(slot);

	if (m_format == GL_RED)
	{
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
	}
	else if (m_format == GL_RG)
	{
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
	}
	else
	{
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
	}
	GLint unpack_alignment = 4;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(m_target,
	             0,
	             internal_format,
	             static_cast<GLsizei>(m_image.width()),
	             static_cast<GLsizei>(m_image.height()),
	             0,
	             m_format,
	             GL_UNSIGNED_BYTE,
	             m_image.pixels().data());
	glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
	m_size = glm::uvec2(m_image.width(), m_image.height());
	return m_size;
}

void Texture::init_dim(const glm::uvec2 size, const GLuint slot)
{
	if (m_id)
	{
		remove();
	}

	m_size = size;
	m_depth = 0;
	m_target = GL_TEXTURE_2D;
	init(slot);

	if ((m_size.x > 0) && (m_size.y > 0))
	{
		glTexStorage2D(m_target, 1, GL_RGBA32F, static_cast<GLsizei>(m_size.x), static_cast<GLsizei>(m_size.y));
		glBindImageTexture(m_slot, m_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}
}

void Texture::init_video_target(const glm::uvec2& size, const GLuint slot)
{
	if (m_id)
	{
		remove();
	}
	m_size = size;
	m_format = GL_RGBA;
	init(slot);
	glTexImage2D(m_target, 0, GL_RGBA8,
	             static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y),
	             0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

void Texture::init_data(const std::vector<uint8_t>& data, const glm::uvec2& size, const GLenum format, const GLuint slot)
{
	if (m_id)
	{
		remove();
	}

	m_size = size;
	m_depth = 1;
	m_target = GL_TEXTURE_2D;
	init(slot);

	if ((m_size.x == 0) && (m_size.y == 0))
	{
		return;
	}
	GLint unpack_alignment = 4;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(m_target,
	             0,
	             internal_format,
	             static_cast<GLsizei>(m_size.x),
	             static_cast<GLsizei>(m_size.y),
	             0, format,
	             GL_UNSIGNED_BYTE,
	             data.data());
	glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
}

void Texture::init_data_3d(const std::vector<uint8_t>& data, const glm::uvec3& size, const GLenum format, const GLuint slot)
{
	if (m_id)
	{
		remove();
	}

	m_size = glm::uvec2(size.x, size.y);
	m_depth = size.z;
	m_target = GL_TEXTURE_3D;
	init(slot);

	if ((size.x == 0) || (size.y == 0) || (size.z == 0))
	{
		return;
	}
	GLint unpack_alignment = 4;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(m_target, 0, internal_format,
	             static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y), static_cast<GLsizei>(size.z),
	             0, format, GL_UNSIGNED_BYTE, data.data());
	glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
}

void Texture::init_sdl(const SDL_Surface* surface, const GLuint slot)
{
	init(slot);
	glTexImage2D(m_target, 0, internal_format, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surface->pixels);
}

void Texture::init_openvr_model(const std::string& name, const GLuint slot)
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

	init(slot);
	glTexImage2D(m_target, 0, internal_format, texture->unWidth, texture->unHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->rubTextureMapData);

	vr::VRRenderModels()->FreeRenderModel(model);
	vr::VRRenderModels()->FreeTexture(texture);
}

GLuint Texture::slot(void) const
{
	return m_slot;
}

void Texture::bind(void) const
{
	glActiveTexture(GL_TEXTURE0 + m_slot);
	glBindTexture(m_target, m_id);
}

void Texture::unbind(void) const
{
	if (m_id)
	{
		glBindTexture(m_target, 0);
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
