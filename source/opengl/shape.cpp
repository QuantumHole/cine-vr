// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define GL_GLEXT_PROTOTYPES

#include "shape.h"
#include <stdexcept>
#include <openvr/openvr.h>
#include <chrono>
#include <thread>
#include <sstream>

Shape::Shape(void) :
	m_vao(),
	m_vbo(),
	m_ebo(),
	m_vbo_instances(),
	m_instances(1),
	m_shape_type(GL_TRIANGLES)
{
}

Shape::Shape(const Shape& s) :
	m_vao(s.m_vao),
	m_vbo(s.m_vbo),
	m_ebo(s.m_ebo),
	m_vbo_instances(s.m_vbo_instances),
	m_instances(s.m_instances),
	m_shape_type(s.m_shape_type)
{
}

Shape& Shape::operator=(const Shape& s)
{
	m_vao = s.m_vao;
	m_vbo = s.m_vbo;
	m_ebo = s.m_ebo;
	m_vbo_instances = s.m_vbo_instances;
	m_instances = s.m_instances;
	m_shape_type = s.m_shape_type;
	return *this;
}

Shape::~Shape(void)
{
	remove();
}

void Shape::remove(void)
{
	m_vao.remove();
	m_vbo.remove();
	m_vbo_instances.remove();
	m_ebo.remove();
}

void Shape::set_instances(const size_t instances)
{
	m_instances = instances;
	std::vector<glm::mat4> transforms(m_instances, glm::mat4(1.0));
	m_vbo_instances.load_data(transforms);
}

size_t Shape::instances(void) const
{
	return m_instances;
}

void Shape::set_transform(const glm::mat4& transform, const size_t instance) const
{
	m_vbo_instances.bind();
	glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(instance * sizeof(glm::mat4)), sizeof(glm::mat4), &transform);
	m_vbo_instances.unbind();
}

void Shape::init_vertices(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const GLenum type)
{
	m_shape_type = type;

	std::vector<glm::mat4> transforms(m_instances, glm::mat4(1.0));

	m_vao.init();
	m_vbo.init();
	m_vbo.load_data(vertices);
	m_vbo_instances.init();
	m_vbo_instances.load_data(transforms);
	m_ebo.init(indices);
	m_vao.link_attrib(m_vbo, 0, 3, GL_FLOAT, sizeof(Vertex), nullptr);
	m_vao.link_attrib(m_vbo, 1, 3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(1 * sizeof(glm::vec3)));
	m_vao.link_attrib(m_vbo, 2, 4, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(2 * sizeof(glm::vec3)));
	m_vao.link_attrib(m_vbo, 3, 2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(2 * sizeof(glm::vec3) + sizeof(glm::vec4)));

	/* mat4 needs to be declared as 4 vec4. */
	m_vao.link_attrib(m_vbo_instances, 4, 4, GL_FLOAT, sizeof(glm::mat4), nullptr);
	m_vao.link_attrib(m_vbo_instances, 5, 4, GL_FLOAT, sizeof(glm::mat4), reinterpret_cast<void*>(1 * sizeof(glm::vec4)));
	m_vao.link_attrib(m_vbo_instances, 6, 4, GL_FLOAT, sizeof(glm::mat4), reinterpret_cast<void*>(2 * sizeof(glm::vec4)));
	m_vao.link_attrib(m_vbo_instances, 7, 4, GL_FLOAT, sizeof(glm::mat4), reinterpret_cast<void*>(3 * sizeof(glm::vec4)));

	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);

	// Bind the VBO and VAO to 0 so that we don't accidentally modify the VAO and VBO we created.
	m_vao.unbind();
	m_vbo.unbind();
	m_vbo_instances.unbind();
	m_ebo.unbind();
}

void Shape::init_openvr_model(const std::string& name)
{
	vr::RenderModel_t* model;
	vr::EVRRenderModelError error;

	while (true)
	{
		error = vr::VRRenderModels()->LoadRenderModel_Async(name.c_str(), &model);

		if (error != vr::VRRenderModelError_Loading)
		{
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if (error != vr::VRRenderModelError_None)
	{
		std::stringstream s;
		s << "Unable to load render model " << name
		  << " - " << vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error);
		throw std::runtime_error(s.str());
	}

	std::vector<glm::mat4> transforms(m_instances, glm::mat4(1.0));

	m_vao.init();
	m_vbo.init();
	m_vbo.load_data(*model);
	m_vbo_instances.init();
	m_vbo_instances.load_data(transforms);
	m_ebo.init(*model);
	m_vao.link_openvr_model(m_vbo);

	/* mat4 needs to be declared as 4 vec4. */
	m_vao.link_attrib(m_vbo_instances, 4, 4, GL_FLOAT, sizeof(glm::mat4), nullptr);
	m_vao.link_attrib(m_vbo_instances, 5, 4, GL_FLOAT, sizeof(glm::mat4), reinterpret_cast<void*>(1 * sizeof(glm::vec4)));
	m_vao.link_attrib(m_vbo_instances, 6, 4, GL_FLOAT, sizeof(glm::mat4), reinterpret_cast<void*>(2 * sizeof(glm::vec4)));
	m_vao.link_attrib(m_vbo_instances, 7, 4, GL_FLOAT, sizeof(glm::mat4), reinterpret_cast<void*>(3 * sizeof(glm::vec4)));

	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);

	// Bind the VBO and VAO to 0 so that we don't accidentally modify the VAO and VBO we created.
	m_vao.unbind();
	m_vbo.unbind();
	m_vbo_instances.unbind();
	m_ebo.unbind();

	vr::VRRenderModels()->FreeRenderModel(model);
}

void Shape::draw(void) const
{
	m_vao.bind();
	m_vbo.bind();
	m_vbo_instances.bind();
	m_ebo.bind();

	glDrawElementsInstanced(m_shape_type,
	                        static_cast<GLsizei>(m_ebo.num_indices()),
	                        m_ebo.index_format(),
	                        nullptr,
	                        static_cast<GLsizei>(m_instances));

	m_vao.unbind();
	m_vbo.unbind();
	m_vbo_instances.unbind();
	m_ebo.unbind();
}
