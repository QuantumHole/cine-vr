// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "progress_bar.h"
#include "application.h"
#include "database/thumbnail_store.h"
#include "font_renderer.h"
#include "util/file_system.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

static const float tex2shape = 0.01f;
static const size_t text_width = 4 * FontRenderer::font_size;

ProgressBar::ProgressBar(
	const action_t action,
	const std::string& imagebar,
	const std::string& imagecursor,
	const float max,
	const float pos,
	const size_t preview,
	const size_t preview_volume,
	const glm::uvec3& preview_size) :
	Panel(action),
	m_progress_max(max),
	m_progress_pos(pos),
	m_renderer(RenderManager::instance()),
	m_background_renderable(0),
	m_cursor_renderable(0),
	m_tooltip_text_renderable(0),
	m_tooltip_image_renderable(std::numeric_limits<size_t>::max()),
	m_background_shape_set(0),
	m_cursor_shape_set(0),
	m_tooltip_text_shape_set(0),
	m_tooltip_image_shape_set(0),
	m_cursor_texture(0),
	m_tooltip_texture(0),
	m_preview_texture(preview_volume),
	m_tooltip_second(std::numeric_limits<size_t>::max()),
	m_preview_slice(0.0f),
	m_preview_size(preview_size),
	m_progress_size(0.0f, 0.0f),
	m_cursor_size(0.0f, 0.0f),
	m_tooltip_size(0.0f, 0.0f),
	m_tooltip_text_size(0.0f, 0.0f),
	m_tooltip_image_size(0.0f, 0.0f),
	m_background_transform(1.0f),
	m_cursor_transform(1.0f),
	m_background_uses_texture(false),
	m_cursor_uses_texture(false)
{
	const std::string extbar = FileSystem().extension(imagebar);

	if (preview > 0)
	{
		m_progress_size = tex2shape * glm::vec2(605.0f, 29.0f);
		m_background_uses_texture = true;
	}
	else if (FileSystem().is_image(extbar))
	{
		init_texture(imagebar);
		m_progress_size = tex2shape * glm::vec2(texture().size());
		m_background_uses_texture = true;
	}
	else if (FileSystem().is_vector(extbar))
	{
		m_background_shape_set = m_renderer.load_svg(imagebar);
		m_progress_size = tex2shape * glm::vec2(605.0f, 29.0f);
		const float svg_scale = 0.5f * m_progress_size.x;
		m_background_transform = glm::scale(glm::mat4(1.0f), glm::vec3(svg_scale, svg_scale, 1.0f));
	}
	else
	{
		throw std::runtime_error("unsupported image format: " + extbar);
	}

	if ((m_progress_size.x <= 0.0f) || (m_progress_size.y <= 0.0f))
	{
		throw std::runtime_error("invalid progress bar dimensions");
	}
	const glm::vec4 color(0.0f);
	init_shape(m_progress_size, color);

	if (m_background_uses_texture)
	{
		m_background_shape_set = Panel::shape_set_id();
	}
	std::vector<size_t> background_textures;

	if (m_background_uses_texture)
	{
		background_textures.push_back(preview > 0 ? preview : texture_id());
	}

	const size_t shader_id = Application::instance().shader().id();
	m_background_renderable = m_renderer.create_shape_set_renderable(m_background_shape_set,
	                                                                 background_textures,
	                                                                 shader_id,
	                                                                 false);

	const std::string extcursor = FileSystem().extension(imagecursor);

	if (FileSystem().is_image(extcursor))
	{
		m_cursor_texture = m_renderer.create_texture();
		Texture& cursor_texture = m_renderer.texture(m_cursor_texture);
		cursor_texture.init_image_file(imagecursor, 0);
		m_cursor_size = tex2shape * glm::vec2(cursor_texture.size());
		m_cursor_uses_texture = true;
		m_cursor_texture = texture_id();
		init_cursor(m_cursor_size);
	}
	else if (FileSystem().is_vector(extcursor))
	{
		m_cursor_shape_set = m_renderer.load_svg(imagecursor);
		m_cursor_size.y = m_progress_size.y * (64.0f / 29.0f);
		m_cursor_size.x = m_cursor_size.y * (277.27591f / 832.0f);
		const float svg_scale = m_cursor_size.y * 0.5f;
		m_cursor_transform = glm::scale(glm::mat4(1.0f), glm::vec3(svg_scale, svg_scale, 1.0f));
		m_cursor_renderable = m_renderer.create_shape_set_renderable(m_cursor_shape_set,
		                                                             std::vector<size_t>(),
		                                                             shader_id,
		                                                             false);
	}
	else
	{
		throw std::runtime_error("unsupported image format: " + extcursor);
	}

	init_tooltip();
	set_cursor_position();
}

ProgressBar::~ProgressBar(void)
{
	m_renderer.delete_renderable(m_background_renderable);
	m_renderer.delete_renderable(m_cursor_renderable);
	m_renderer.delete_renderable(m_tooltip_text_renderable);

	if (m_tooltip_image_renderable != std::numeric_limits<size_t>::max())
	{
		m_renderer.delete_renderable(m_tooltip_image_renderable);
	}
}

void ProgressBar::set_transform(const glm::mat4& pose)
{
	Panel::set_transform(pose);
	m_renderer.set_shape_set_transform(m_background_shape_set, pose * m_background_transform);
	set_cursor_position();
}

void ProgressBar::set_max(const float max)
{
	m_progress_max = max;
}

void ProgressBar::set_pos(const float pos)
{
	m_progress_pos = pos;
}

void ProgressBar::set_cursor_position(void)
{
	if (m_progress_max <= 0.0f)
	{
		return;
	}
	m_progress_pos = Application::instance().player().playtime();
	const float frac = glm::clamp(m_progress_pos / m_progress_max, 0.0f, 1.0f);
	const float shift = (frac - 0.5f) * m_progress_size.x;

	// cursor hovers slightly above the progress bar to avoid z-fighting
	glm::mat4 shifted_pose = glm::translate(Panel::pose(), glm::vec3(shift, 0.0f, 1e-4f));
	m_renderer.set_shape_set_transform(m_cursor_shape_set, shifted_pose * m_cursor_transform);
}

void ProgressBar::init_cursor(const glm::vec2& cursor_size)
{
	const float eps = 1e-4f;

	const std::vector<Vertex> svertices = {
		Vertex(glm::vec3(-0.5f * cursor_size.x, -0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.5f * cursor_size.x,  0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f * cursor_size.x,  0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.5f * cursor_size.x, -0.5f * cursor_size.y, eps), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<GLuint> sindices = {
		0, 1, 2,
		0, 3, 1,
	};

	m_cursor_shape_set = m_renderer.create_shape_set();
	m_renderer.add_shape(m_cursor_shape_set, svertices, sindices);
	std::vector<size_t> cursor_textures;

	if (m_cursor_uses_texture)
	{
		cursor_textures.push_back(m_cursor_texture);
	}

	const size_t shader_id = Application::instance().shader().id();
	m_cursor_renderable = m_renderer.create_shape_set_renderable(m_cursor_shape_set,
	                                                             cursor_textures,
	                                                             shader_id,
	                                                             false);
}

void ProgressBar::init_tooltip(void)
{
	m_tooltip_text_size = 0.003f * glm::vec2(static_cast<float>(text_width), static_cast<float>(FontRenderer::font_size));

	if (m_preview_texture > 0)
	{
		m_tooltip_image_size = 0.003f * glm::vec2(static_cast<float>(m_preview_size.x), static_cast<float>(m_preview_size.y));
	}
	m_tooltip_size = (m_tooltip_image_size.x > 0.0f) ? m_tooltip_image_size : m_tooltip_text_size;
	const std::vector<GLuint> indices = {0, 1, 2, 0, 3, 1};
	const size_t shader_id = Application::instance().shader().id();
	std::vector<uint8_t> pixels(text_width * FontRenderer::font_size * sizeof(uint32_t), 0);
	for (size_t pixel = 0; pixel < text_width * FontRenderer::font_size; pixel++)
	{
		pixels[pixel * 4] = 24;
		pixels[pixel * 4 + 1] = 24;
		pixels[pixel * 4 + 2] = 24;
		pixels[pixel * 4 + 3] = 128;
	}
	m_tooltip_texture = m_renderer.add_texture(
		pixels,
		glm::uvec2(static_cast<GLuint>(text_width), static_cast<GLuint>(FontRenderer::font_size)),
		GL_RGBA,
		0);
	const std::vector<Vertex> text_vertices = {
		Vertex(glm::vec3(-0.5f * m_tooltip_text_size.x, -0.5f * m_tooltip_text_size.y, 1e-4f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.5f * m_tooltip_text_size.x,  0.5f * m_tooltip_text_size.y, 1e-4f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f * m_tooltip_text_size.x,  0.5f * m_tooltip_text_size.y, 1e-4f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.5f * m_tooltip_text_size.x, -0.5f * m_tooltip_text_size.y, 1e-4f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 1.0f)),
	};
	m_tooltip_text_shape_set = m_renderer.create_shape_set();
	m_renderer.add_shape(m_tooltip_text_shape_set, text_vertices, indices);
	m_tooltip_text_renderable = m_renderer.create_shape_set_renderable(
		m_tooltip_text_shape_set, std::vector<size_t>(1, m_tooltip_texture), shader_id, false);
	m_renderer.enable(m_tooltip_text_renderable, false);

	if (m_preview_texture > 0)
	{
		const std::vector<Vertex> image_vertices = {
			Vertex(glm::vec3(-0.5f * m_tooltip_image_size.x, -0.5f * m_tooltip_image_size.y, 1e-4f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 1.0f)),
			Vertex(glm::vec3( 0.5f * m_tooltip_image_size.x,  0.5f * m_tooltip_image_size.y, 1e-4f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 0.0f)),
			Vertex(glm::vec3(-0.5f * m_tooltip_image_size.x,  0.5f * m_tooltip_image_size.y, 1e-4f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 0.0f)),
			Vertex(glm::vec3( 0.5f * m_tooltip_image_size.x, -0.5f * m_tooltip_image_size.y, 1e-4f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 1.0f)),
		};
		m_tooltip_image_shape_set = m_renderer.create_shape_set();
		m_renderer.add_shape(m_tooltip_image_shape_set, image_vertices, indices);
		m_tooltip_image_renderable = m_renderer.create_shape_set_renderable(
			m_tooltip_image_shape_set, std::vector<size_t>(1, m_preview_texture), shader_id, false);
		m_renderer.enable(m_tooltip_image_renderable, false);
	}
}

void ProgressBar::update_tooltip(const float position)
{
	if (m_preview_size.z > 0)
	{
		m_preview_slice = std::min(1.0f, position / m_progress_max);
	}
	const unsigned long long total_seconds = static_cast<unsigned long long>(std::max(0.0f, position));
	const size_t second = static_cast<size_t>(total_seconds);

	if (second != m_tooltip_second)
	{
		const unsigned long long hours = total_seconds / 3600;
		const unsigned int minutes = static_cast<unsigned int>((total_seconds / 60) % 60);
		const unsigned int seconds = static_cast<unsigned int>(total_seconds % 60);
		std::ostringstream text;
		text << std::setfill('0')
			 << std::setw(2) << hours << ":"
		     << std::setw(2) << minutes << ":"
			 << std::setw(2) << seconds;

		std::vector<uint32_t> glyphs;
		FontRenderer::instance().render_text(text.str(), glyphs);
		const size_t glyph_width = glyphs.size() / FontRenderer::font_size;
		const size_t visible_width = std::min<size_t>(glyph_width, text_width);
		std::vector<uint8_t> pixels(text_width * FontRenderer::font_size * sizeof(uint32_t), 0);
		for (size_t pixel = 0; pixel < pixels.size(); pixel+=4)
		{
			pixels[pixel + 0] = 24;
			pixels[pixel + 1] = 24;
			pixels[pixel + 2] = 24;
			pixels[pixel + 3] = 160;
		}
		const uint8_t* glyph_data = reinterpret_cast<const uint8_t*>(glyphs.data());
		const size_t left = (text_width - visible_width) / 2;

		for (size_t row = 0; row < FontRenderer::font_size; row++)
		{
			const size_t source_offset = row * glyph_width * sizeof(uint32_t);
			const size_t target_offset = (row * text_width + left) * sizeof(uint32_t);
			for (size_t column = 0; column < 4 * visible_width; column += 4)
			{
				const uint8_t coverage = glyph_data[source_offset + column];

				if (coverage > 0)
				{
					pixels[target_offset + column + 0] = coverage;
					pixels[target_offset + column + 1] = coverage;
					pixels[target_offset + column + 2] = coverage;
					pixels[target_offset + column + 3] = 255;
				}
			}
		}

		m_renderer.texture(m_tooltip_texture).init_data(
			pixels, glm::uvec2(text_width, FontRenderer::font_size), GL_RGBA, 0);
		m_tooltip_second = second;
	}

	const float fraction = (m_progress_max > 0.0f)
	    ? glm::clamp(position / m_progress_max, 0.0f, 1.0f)
	    : 0.0f;
	const float max_shift = 0.5f * (m_progress_size.x - m_tooltip_size.x);
	const float shift = glm::clamp((fraction - 0.5f) * m_progress_size.x, -max_shift, max_shift);
	const float height = 0.5f * (m_progress_size.y + m_tooltip_size.y) + 0.02f;
	const glm::mat4 tooltip_pose = glm::translate(Panel::pose(), glm::vec3(shift, height, 2e-4f));
	const float text_y = -0.5f * (m_tooltip_image_size.y - m_tooltip_text_size.y);
	const glm::mat4 text_pose = glm::translate(tooltip_pose, glm::vec3(0.0f, text_y, 1e-3f));
	m_renderer.set_shape_set_transform(m_tooltip_text_shape_set, text_pose);

	if (m_preview_texture > 0)
	{
		m_renderer.set_shape_set_transform(m_tooltip_image_shape_set, tooltip_pose);
		m_renderer.enable(m_tooltip_image_renderable, true);
	}
	m_renderer.enable(m_tooltip_text_renderable, true);
}

void ProgressBar::draw(void) const
{
	ShaderSet& shader = Application::instance().shader();

	shader.set_uniform("background", m_background_uses_texture);
	m_renderer.draw(m_background_renderable);
	shader.set_uniform("background", m_cursor_uses_texture);
	m_renderer.draw(m_cursor_renderable);
	shader.set_uniform("background", true);

	if (m_preview_texture > 0)
	{
		shader.set_uniform("preview3d", 1);
		shader.set_uniform("use_preview3d", 1);
		shader.set_uniform("preview_slice", m_preview_slice);
		m_renderer.draw(m_tooltip_image_renderable);
		shader.set_uniform("use_preview3d", 0);
	}
	m_renderer.draw(m_tooltip_text_renderable);
	shader.set_uniform("preview3d", 0);
	shader.set_uniform("background", false);
}

void ProgressBar::update_state(void)
{
	m_renderer.enable(m_tooltip_text_renderable, false);
	set_cursor_position();
}

bool ProgressBar::update_on_interaction(const intersection_t isec, const OpenVRInterface::input_state_t& input)
{
	if (isec.hit && (m_progress_max > 0.0f))
	{
		update_tooltip(isec.local.x * m_progress_max);
	}
	else
	{
		m_renderer.enable(m_tooltip_text_renderable, false);

		if (m_tooltip_image_renderable != std::numeric_limits<size_t>::max())
		{
			m_renderer.enable(m_tooltip_image_renderable, false);
		}
	}

	if (input.trigger.button.released && isec.hit)
	{
		const float step = m_progress_max * isec.local.x - m_progress_pos;
		Application::instance().player().jump(step);
	}

	return input.trigger.button.released && isec.hit;
}
