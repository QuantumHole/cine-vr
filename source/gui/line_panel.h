// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LINE_PANEL_H
#define LINE_PANEL_H

#include "action.h"
#include "opengl/shape.h"
#include "scroll_panel.h"
#include <vector>
#include <string>

class LinePanel : public ScrollPanel
{
	private:
		typedef struct
		{
			size_t renderable;
			std::string content;
		}
		line_entry_t;

		const std::string m_title;
		std::vector<line_entry_t> m_lines;
		size_t m_active_line;
		Panel m_title_bar;
		Shape m_selection_bar;

		void init_text_bar(Shape& bar);

	public:
		static const size_t line_height;
		static const size_t texture_width;

		explicit LinePanel(const action_t action, const std::string& title);
		~LinePanel(void) override;

		void set_transform(const glm::mat4& pose) override;

		void clear_lines(void);
		void add_line(const size_t renderable, const std::string& content);
		void render_lines(void);

		const std::string get_selection(void) const;

		bool update_on_interaction(const intersection_t isec, const OpenVRInterface::input_state_t& input) override;
		void draw(void) const override;
};

#endif
