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
		size_t m_background_renderable;
		size_t m_cursor_renderable;
		size_t m_tooltip_text_renderable;
		size_t m_tooltip_image_renderable;
		size_t m_background_shape_set;
		size_t m_cursor_shape_set;
		size_t m_tooltip_text_shape_set;
		size_t m_tooltip_image_shape_set;
		size_t m_cursor_texture;
		size_t m_tooltip_texture;
		size_t m_preview_texture;
		size_t m_tooltip_second;
		float m_preview_slice;
		glm::uvec3 m_preview_size;
		glm::vec2 m_progress_size;
		glm::vec2 m_cursor_size;
		glm::vec2 m_tooltip_size;
		glm::vec2 m_tooltip_text_size;
		glm::vec2 m_tooltip_image_size;
		glm::mat4 m_background_transform;
		glm::mat4 m_cursor_transform;
		bool m_background_uses_texture;
		bool m_cursor_uses_texture;

		void init_cursor(const glm::vec2& cursor_size);
		void init_tooltip(void);
		void update_tooltip(const float position);
		void set_cursor_position(void);

	public:
		/** constructor.
		 * @param action the action ID reported when the progress bar is clicked.
		 * @param imagebar file name of the default progress bar texture.
		 * @param imagecursor file name of the progress bar cursor.
		 * @param max value reported at the end of the progress bar.
		 * @param pos current position of the progress bar cursor.
		 * @param preview texture ID of an alternative bar texture, when available, or 0.
		 */
		explicit ProgressBar(
			const action_t action,
			const std::string& imagebar,
			const std::string& imagecursor,
			const float max,
			const float pos,
			const size_t preview,
			const size_t preview_volume,
			const glm::uvec3& preview_size);
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
