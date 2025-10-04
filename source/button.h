// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BUTTON_H
#define BUTTON_H

#include "opengl/shape.h"
#include "opengl/texture.h"

class Button
{
	public:
		typedef enum
		{
			BUTTON_NONE,
			BUTTON_BACKWARD,
			BUTTON_CUBE_MONO,
			BUTTON_CUBE_STEREO,
			BUTTON_CYLINDER,
			BUTTON_DELETE,
			BUTTON_FISHEYE,
			BUTTON_FLAT,
			BUTTON_FORWARD,
			BUTTON_LEFT_RIGHT,
			BUTTON_MONO,
			BUTTON_NEXT,
			BUTTON_OPEN,
			BUTTON_PAUSE,
			BUTTON_PLAY,
			BUTTON_POWER,
			BUTTON_PREVIOUS,
			BUTTON_SPHERE,
			BUTTON_TOP_BOTTOM
		}
		button_action_t;

		typedef struct
		{
			size_t button_id;
			button_action_t action_id;
			bool hit;
			glm::vec3 global;
			glm::vec2 local;    // texture coordinates
		}
		intersection_t;

		explicit Button(void);
		void init(const float size, const button_action_t action);
		void enable(const bool active);
		void set_transform(const glm::mat4& pose);
		intersection_t intersection(const glm::mat4& pose) const;
		void draw(void) const;

	private:
		const size_t m_id;                // OpenGL ID
		button_action_t m_action;         // button function indicator
		bool m_active;                    // flag for possible interactions
		Shape m_shape;                    // mesh
		Texture m_tex;                    // optional texture
		glm::vec2 m_size;                 // size in scene coordinate space
		glm::mat4 m_pose;                 // position and rotation
};

#endif
