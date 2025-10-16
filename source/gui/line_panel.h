// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LINE_PANEL_H
#define LINE_PANEL_H

#include "action.h"
#include "panel.h"
#include <vector>
#include <string>

class LinePanel : public Panel
{
	private:
		typedef struct
		{
			std::string text;
			std::string content;
			size_t level;
		}
		line_entry_t;

		std::vector<line_entry_t> m_lines;
		size_t m_active_line;
		Shape m_selection_bar;

		void init_selection(void);

	protected:

	public:
		explicit LinePanel(const action_t action);
		~LinePanel(void) override;

		void add_line(const std::string& text, const std::string& content = "", const size_t level = 0);

		bool update_on_interaction(const intersection_t isec, const bool pressed, const bool released) override;
		void draw(void) const override;
};

#endif
