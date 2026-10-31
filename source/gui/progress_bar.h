// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include "panel.h"
#include "opengl/render_manager.h"

class ProgressBar : public Panel
{
	private:
		float m_progress_max;
		float m_progress_pos;
		RenderManager& m_renderer;
		RenderManager::RenderableId m_background_renderable;
		RenderManager::RenderableId m_cursor_renderable;
		size_t m_background_shape_set;
		size_t m_cursor_shape_set;
		size_t m_cursor_texture;
		glm::vec2 m_progress_size;
		glm::vec2 m_cursor_size;
		glm::mat4 m_background_transform;
		glm::mat4 m_cursor_transform;
		bool m_background_uses_texture;
		bool m_cursor_uses_texture;

		void init_cursor(const glm::vec2& cursor_size);
		void set_cursor_position(void);

	public:
		explicit ProgressBar(
			const action_t action,
			const std::string& imagebar,
			const std::string& imagecursor,
			const float max,
			const float pos);
		~ProgressBar(void) override;
		void set_transform(const glm::mat4& pose) override;
		void set_max(const float max);
		void set_pos(const float pos);

		float position(void) const;
		void update_state(void) override;
		bool update_on_interaction(const Panel::intersection_t isec, const OpenVRInterface::input_state_t& input) override;
		void draw(void) const override;
};

#endif
