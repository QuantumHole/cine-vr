// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BUTTON_H
#define BUTTON_H

#include "opengl/shape.h"
#include "opengl/texture.h"
#include "action.h"

class Button
{
	public:
		typedef struct
		{
			action_t action_id;
			bool hit;
			glm::vec3 global;
			glm::vec2 local;    // texture coordinates
		}
		intersection_t;

		explicit Button(const action_t action, const std::string& image_name);
		explicit Button(const action_t action, const std::string& image_name, const bool value);
		explicit Button(const action_t action, const std::string& image_name, const float min, const float max, const float value);
		~Button(void);

		bool toggleable(void) const;
		void enable(const bool active);
		bool active(void) const;
		bool slideable(void) const;
		float slide_value(void) const;
		void update_slide_value(const float pos);
		void set_transform(const glm::mat4& pose);
		intersection_t intersection(const glm::mat4& pose) const;
		void draw(void) const;

	private:
		static const glm::vec2 m_size;    // size in scene coordinate space

		action_t m_action;                // button function indicator
		bool m_toggleable;                // enable on/off behaviour
		bool m_active;                    // flag for possible interactions or visible slidebar
		bool m_slideable;                 // enable slidebar behaviour
		float m_slide_min;                // minimum slidebar value
		float m_slide_max;                // maximum slidebar value
		float m_slide_pos;                // current slidebar value
		float m_slide_last;               // original slidebar value
		Shape m_shape;                    // mesh
		Shape m_slidebar;                 // slidebar shape when this button is slideable
		Texture m_tex;                    // optional texture
		glm::mat4 m_pose;                 // position and rotation

		void init_shape(const std::string& image_name);
		void init_slidebar(void);
};

#endif
