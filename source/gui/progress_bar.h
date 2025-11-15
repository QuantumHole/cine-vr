// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include "panel.h"

class ProgressBar : public Panel
{
	private:
		float m_progress_max;
		float m_progress_pos;
		Shape m_cursor;
		Texture m_cursor_tex;

		void init_cursor(void);
		void set_cursor_position(void);

	public:
		explicit ProgressBar(const action_t action, const std::string& image, const float max, const float pos);
		~ProgressBar(void) override;
		void set_max(const float max);
		void set_pos(const float pos);

		float position(void) const;
		bool update_on_interaction(const Panel::intersection_t isec, const OpenVRInterface::input_state_t& input) override;
		void draw(void) const override;
};

#endif
