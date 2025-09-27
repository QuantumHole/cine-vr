#include "controller.h"

Controller::Controller(void) :
	m_body(),
	m_line()
{
}

Controller::Controller(const Controller& c) :
	m_body(c.m_body),
	m_line(c.m_line)
{

}

Controller& Controller::operator=(const Controller& c)
{
	m_body = c.m_body;
	m_line = c.m_line;
	return *this;
}

void Controller::init_line(void)
{
	// simple line with two vertices, will update dynamically
	std::vector<Vertex> vertices = {
		Vertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.1f, 0.9f, 0.1f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.1f, 0.9f, 0.1f), glm::vec2(0.0f, 0.0f)),
	};

	const std::vector<GLuint> indices = {
		0, 1
	};

	m_line.init_vertices(vertices, indices, GL_LINES);
}

void Controller::init(void)
{
	init_line();
}

void Controller::set_transform(const glm::mat4& pose)
{
	// m_body.set_transform(pose);
	m_line.set_transform(pose);
}

void Controller::draw(void) const
{
	// m_body.draw();
	m_line.draw();
}
