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
			BUTTON_FILE_DELETE,
			BUTTON_FILE_OPEN,
			BUTTON_PLAY_BACKWARD,
			BUTTON_PLAY_FORWARD,
			BUTTON_PLAY_NEXT,
			BUTTON_PLAY_PAUSE,
			BUTTON_PLAY_PLAY,
			BUTTON_PLAY_PREVIOUS,
			BUTTON_POWER,
			BUTTON_PROJECT_CUBE,
			BUTTON_PROJECT_CYLINDER,
			BUTTON_PROJECT_FISHEYE,
			BUTTON_PROJECT_FLAT,
			BUTTON_PROJECT_SPHERE,
			BUTTON_TILE_CUBE_MONO,
			BUTTON_TILE_CUBE_STEREO,
			BUTTON_TILE_LEFT_RIGHT,
			BUTTON_TILE_MONO,
			BUTTON_TILE_TOP_BOTTOM,
			BUTTON_FLAG_MONO,
			BUTTON_FLAG_STRETCH,
			BUTTON_FLAG_SWITCH_EYES,
			BUTTON_PARAM_ANGLE,
			BUTTON_PARAM_ZOOM
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
		void init(const float size, const button_action_t action, const bool toggleable = false);
		void enable(const bool active);
		bool active(void) const;
		void set_transform(const glm::mat4& pose);
		intersection_t intersection(const glm::mat4& pose) const;
		void draw(void) const;

	private:
		const size_t m_id;                // OpenGL ID
		button_action_t m_action;         // button function indicator
		bool m_toggleable;                // enable on/off behaviour
		bool m_active;                    // flag for possible interactions
		Shape m_shape;                    // mesh
		Texture m_tex;                    // optional texture
		glm::vec2 m_size;                 // size in scene coordinate space
		glm::mat4 m_pose;                 // position and rotation
};

#endif
