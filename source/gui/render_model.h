// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RENDER_MODEL_H
#define RENDER_MODEL_H

#include <string>
#include <openvr.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "opengl/shape.h"
#include "opengl/texture.h"

class RenderModel
{
	private:
		Shape m_shape;
		Texture m_tex;
		glm::vec3 m_position;
		glm::quat m_rotation;

	public:
		RenderModel(void);
		RenderModel(const RenderModel& rm);
		RenderModel& operator=(const RenderModel& rm);
		~RenderModel(void);

		void init_openvr_model(const std::string& name);
		void cleanup(void);
		void draw(void) const;

		void set_transform(const glm::mat4& matrix);
		void set_transform(const glm::vec3& position, const glm::quat& rotation);
};

#endif
