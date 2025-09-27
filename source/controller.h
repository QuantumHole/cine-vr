// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "opengl/shape.h"
#include "render_model.h"

class Controller
{
	private:
		RenderModel m_body;
		Shape m_line;

		void init_line(void);
		void init_point(void);

	public:
		explicit Controller(void);
		Controller(const Controller& c);
		Controller& operator=(const Controller& c);

		void init(const std::string& name);
		void set_transform(const glm::mat4& pose);
		void draw(void) const;
};

#endif
