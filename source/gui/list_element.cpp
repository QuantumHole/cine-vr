// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "list_element.h"

#include "gui/line_panel.h"
#include "gui/font_renderer.h"
#include "opengl/render_manager.h"
#include "application.h"

ListElement::ListElement(void)
{
}

size_t ListElement::create_thumbnail_label(const std::string& text, const bool align_right, const bool top_half) const
{
	std::vector<uint32_t> glyphs;

	FontRenderer::instance().render_text(text, glyphs);
	const size_t glyph_width = glyphs.size() / FontRenderer::font_size;
	const size_t visible_width = std::min(glyph_width, LinePanel::texture_width);
	const size_t texture_width = std::max<size_t>(1, visible_width);
	std::vector<uint8_t> image(texture_width * FontRenderer::font_size* sizeof(uint32_t), 0);

	if (visible_width > 0)
	{
		const uint8_t* glyph_data = reinterpret_cast<const uint8_t*>(glyphs.data());
		for (size_t row = 0; row < FontRenderer::font_size; row++)
		{
			const size_t source_offset = row * glyph_width * sizeof(uint32_t);
			const size_t target_offset = row * texture_width * sizeof(uint32_t);
			std::copy(glyph_data + source_offset,
			          glyph_data + source_offset + visible_width * sizeof(uint32_t),
			          image.begin() + static_cast<std::ptrdiff_t>(target_offset));
		}
	}

	RenderManager& renderer = RenderManager::instance();
	const size_t texture = renderer.add_texture(
		image,
		glm::uvec2(static_cast<GLuint>(texture_width), static_cast<GLuint>(FontRenderer::font_size)),
		GL_RGBA,
		0);
	const float top = top_half ? 0.5f : 0.0f;
	const float bottom = top_half ? 0.0f : -0.5f;
	const float display_width = 0.5f * static_cast<float>(visible_width) / static_cast<float>(LinePanel::texture_width);
	const float left = align_right ? 0.5f - display_width : -0.5f;
	const float right = left + display_width;
	const std::vector<Vertex> vertices = {
		Vertex(glm::vec3(left,  bottom, 0.001f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3(right, top,    0.001f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(left,  top,    0.001f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3(right, bottom, 0.001f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 1.0f)),
	};
	const std::vector<GLuint> indices = {0, 1, 2, 0, 3, 1};
	return renderer.create_shape_renderable(
		vertices,
		indices,
		std::vector<size_t>(1, texture),
		Application::instance().shader().id(),
		1,
		GL_TRIANGLES,
		false);
}

size_t ListElement::create_text_element(const std::string& text, const size_t indent_level)
{
	const size_t level_offset = LinePanel::line_height / 2;
	const size_t indent = std::min(indent_level, LinePanel::texture_width / level_offset) * level_offset;
	std::vector<uint8_t> image(LinePanel::texture_width* LinePanel::line_height* sizeof(uint32_t), 0);

	std::vector<uint32_t> glyphs;

	FontRenderer::instance().render_text(text, glyphs);
	const size_t glyph_width = glyphs.size() / LinePanel::line_height;
	const size_t visible_width =
		(indent < LinePanel::texture_width)
		? std::min(glyph_width, LinePanel::texture_width - indent)
		: 0;

	if (visible_width > 0)
	{
		const uint8_t* glyph_data = reinterpret_cast<const uint8_t*>(glyphs.data());
		for (size_t row = 0; row < LinePanel::line_height; row++)
		{
			const size_t source_offset = row * glyph_width * sizeof(uint32_t);
			const size_t target_offset = (row * LinePanel::texture_width + indent) * sizeof(uint32_t);
			std::copy(glyph_data + source_offset,
			          glyph_data + source_offset + visible_width * sizeof(uint32_t),
			          image.begin() + static_cast<std::ptrdiff_t>(target_offset));
		}
	}

	RenderManager& renderer = RenderManager::instance();
	const size_t texture = renderer.create_texture();
	renderer.texture(texture).init_data(image, glm::uvec2(LinePanel::texture_width, LinePanel::line_height), GL_RGBA, 0);

	const size_t shape_set = renderer.create_shape_set();
	const std::vector<Vertex> vertices = {
		Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.5f,  0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f,  0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 1.0f)),
	};
	const std::vector<GLuint> indices = {0, 1, 2, 0, 3, 1};
	renderer.add_shape(shape_set, vertices, indices);
	const size_t renderable = renderer.create_shape_set_renderable(shape_set,
	                                                               std::vector<size_t>(1, texture),
	                                                               Application::instance().shader().id(),
	                                                               false);

	return renderable;
}

size_t ListElement::create_video_element(
	const std::string& title,
	const std::string& subtitle,
	const Thumbnail& background) const
{
	std::vector<uint8_t> image(LinePanel::texture_width* LinePanel::line_height* sizeof(uint32_t), 0);

	RenderManager& renderer = RenderManager::instance();
	const size_t background_texture = renderer.add_texture(
		background.pixels,
		glm::uvec2(static_cast<GLuint>(background.width), static_cast<GLuint>(background.height)),
		GL_RGB,
		0);
	const std::vector<Vertex> background_vertices = {
		Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( 0.5f,  0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-0.5f,  0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 1.0f)),
	};
	const std::vector<GLuint> background_indices = {0, 1, 2, 0, 3, 1};
	const size_t background_renderable = renderer.create_shape_renderable(
		background_vertices,
		background_indices,
		std::vector<size_t>(1, background_texture),
		Application::instance().shader().id());
	const size_t title_renderable = create_thumbnail_label(title, false, true);
	const size_t subtitle_renderable = create_thumbnail_label(subtitle, true, false);

	return renderer.create_group({background_renderable, title_renderable, subtitle_renderable}, false);
}
