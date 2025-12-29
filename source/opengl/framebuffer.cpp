// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define GL_GLEXT_PROTOTYPES

#include <iostream>
#include "framebuffer.h"

Framebuffer::Framebuffer(void) :
	m_framebuffer_id(0),
	m_texture_id(0),
	m_depthbuffer_id(0),
	m_size(0, 0)
{
}

void Framebuffer::init(const glm::uvec2& size)
{
	m_size = size;
	glGenFramebuffers(1, &m_framebuffer_id);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_id);

	glGenTextures(1, &m_texture_id);
	glBindTexture(GL_TEXTURE_2D, m_texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(m_size.x), static_cast<GLsizei>(m_size.y), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_id, 0);

	glGenRenderbuffers(1, &m_depthbuffer_id);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthbuffer_id);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, static_cast<GLsizei>(m_size.x), static_cast<GLsizei>(m_size.y));
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer_id);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "Framebuffer incomplete: " << status << std::endl;
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Framebuffer::id(void) const
{
	return m_framebuffer_id;
}

GLuint Framebuffer::texture(void) const
{
	return m_texture_id;
}

const glm::uvec2& Framebuffer::size(void) const
{
	return m_size;
}

void Framebuffer::bind(const GLenum mode) const
{
	glBindFramebuffer(mode, m_framebuffer_id);
}

/** binds the default framebuffer.
 * @param mode framebuffer mode GL_READ_FRAMEBUFFER or GL_DRAW_FRAMEBUFFER.
 */
void Framebuffer::unbind(const GLenum mode) const
{
	glBindFramebuffer(mode, 0);
}
