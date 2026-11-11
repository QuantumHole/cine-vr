// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SIMPLE_BUTTON_H
#define SIMPLE_BUTTON_H

#include "opengl/shape.h"
#include "opengl/render_manager.h"
#include "opengl/texture.h"
#include "action.h"
#include "panel.h"

class SimpleButton : public Panel
{
	protected:
		static const glm::vec2 m_button_size;
		RenderManager& m_renderer;
		size_t m_renderable_id;
		size_t m_shape_set_id;
		glm::mat4 m_graphic_transform;
		bool m_graphic_uses_texture;

		void set_graphic_pose(const glm::mat4& pose);

	public:
		explicit SimpleButton(const action_t action, const std::string& image_name);
		~SimpleButton(void) override;
		void set_graphic(const size_t shape_set_id,
		                 const bool use_texture = false,
		                 const glm::mat4& local_transform = glm::mat4(1.0f));
		void set_transform(const glm::mat4& pose) override;

		virtual bool update_on_interaction(const Panel::intersection_t isec, const OpenVRInterface::input_state_t& input) override;
		virtual void draw(void) const override;
};

#endif
