// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCROLL_PANEL_H
#define SCROLL_PANEL_H

#include "panel.h"

class ScrollPanel : public Panel
{
	private:
		glm::uvec2 m_tex_offset;
		glm::uvec2 m_tex_view;

	protected:

	public:
		explicit ScrollPanel(const action_t action);

		void set_view_size(const glm::uvec2& view);

		virtual void draw(void) const override;

		virtual bool update_on_interaction(const intersection_t isec, const bool pressed, const bool released) override;
};

#endif
