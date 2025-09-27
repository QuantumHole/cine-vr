// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BUTTON_H
#define BUTTON_H

#include "opengl/shape.h"

class Button
{
	private:
		const size_t m_id;
		Shape m_shape;
		glm::vec2 m_size;
		glm::mat4 m_pose;

	public:
		typedef struct
		{
			size_t button_id;
			bool hit;
			glm::vec3 global;
			glm::vec2 local;    // texture coordinates
		}
		intersection_t;

		explicit Button(void);
		void init(const glm::vec2& size);
		void set_transform(const glm::mat4& pose);
		intersection_t intersection(const glm::mat4& pose) const;
		const Shape& shape(void) const;
};

#endif
