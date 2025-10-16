// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PANEL_H
#define PANEL_H

#include "action.h"
#include "opengl/shape.h"
#include "opengl/texture.h"
#include <vector>

class Panel
{
	private:
		Shape m_shape;
		Texture m_texture;
		glm::mat4 m_pose;                 // position and rotation
		glm::vec2 m_shape_size;
		glm::uvec2 m_tex_size;

	protected:
		const action_t m_action;          // function indicator

		const glm::mat4& pose(void) const;
		const Shape& shape(void) const;
		const Texture& texture(void) const;

		void init_shape(const glm::vec2& shape_size, const glm::vec4& color);
		void init_texture(const std::string& image_name);
		void init_texture(const glm::uvec2& tex_size);

	public:
		typedef struct
		{
			action_t action_id;
			bool hit;
			glm::vec3 global;
			glm::vec2 local;              // texture coordinates
		}
		intersection_t;

		explicit Panel(const action_t action);
		virtual ~Panel(void);

		void init_area(const glm::vec2& shape_size, const glm::vec4& color, const glm::uvec2& tex_size);
		void set_transform(const glm::mat4& pose);
		void text(const std::string& text, const int32_t x = 0, const int32_t y = 0) const;
		virtual void draw(void) const;

		virtual intersection_t intersection(const glm::mat4& pose) const;
		virtual bool update_on_interaction(const intersection_t isec, const bool pressed, const bool released);
};

#endif
