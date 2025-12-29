// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define GL_GLEXT_PROTOTYPES

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <GL/gl.h>
#include <GL/glext.h>
#include <map>
#include <stdexcept>
#include <sstream>
#include "shader_set.h"

ShaderSet::ShaderSet(void) :
	m_program_id(0)
{
}

ShaderSet::~ShaderSet(void)
{
	if (m_program_id)
	{
		glDeleteProgram(static_cast<GLuint>(m_program_id));
	}
}

size_t ShaderSet::id(void) const
{
	return m_program_id;
}

float ShaderSet::get_glsl_version(void) const
{
	float glsl_version;

	std::stringstream s;

	s << glGetString(GL_SHADING_LANGUAGE_VERSION);
	std::cout << "GLSL version: " << s.str() << std::endl;
	s >> glsl_version;
	return glsl_version;
}

void ShaderSet::check_program_log(const size_t program, const std::string& operation) const
{
	if (!glIsProgram(static_cast<GLuint>(program)))
	{
		std::cerr << "invalid program" << std::endl;
	}

	int maxLength = 0;

	glGetProgramiv(static_cast<GLuint>(program), GL_INFO_LOG_LENGTH, &maxLength);

	int infoLogLength = 0;
	std::vector<char> info_log(static_cast<size_t>(maxLength));

	glGetProgramInfoLog(static_cast<GLuint>(program), maxLength, &infoLogLength, &info_log[0]);

	std::cerr << "program " << operation << " failed";

	if (infoLogLength > 0)
	{
		std::cerr << ": " << std::endl << &info_log[0];
	}
	std::cerr << std::endl;
}

void ShaderSet::check_shader_log(const size_t shader, const std::string& operation) const
{
	if (!glIsShader(static_cast<GLuint>(shader)))
	{
		std::cerr << "invalid shader" << std::endl;
	}

	int maxLength = 0;

	glGetShaderiv(static_cast<GLuint>(shader), GL_INFO_LOG_LENGTH, &maxLength);

	int infoLogLength = 0;
	std::vector<char> info_log(static_cast<size_t>(maxLength));

	glGetShaderInfoLog(static_cast<GLuint>(shader), maxLength, &infoLogLength, &info_log[0]);

	std::cerr << "shader " << operation << " failed";

	if (infoLogLength > 0)
	{
		std::cerr << ": " << std::endl << &info_log[0];
	}
	std::cerr << std::endl;
}

void ShaderSet::check_error(const std::string& operation) const
{
	// Check for error
	GLenum error = glGetError();

	if (error != GL_NO_ERROR)
	{
		std::cout << "Error " << operation << " shader: " << error << std::endl;
		check_program_log(m_program_id, operation);
	}
}

std::set<std::string> ShaderSet::get_uniforms(void) const
{
	std::set<std::string> names;

	GLint count;

	glGetProgramiv(static_cast<GLuint>(m_program_id), GL_ACTIVE_UNIFORMS, &count);

	for (GLint i = 0; i < count; i++)
	{
		GLchar buffer[32];
		GLenum type;
		GLsizei length;
		GLint size;
		glGetActiveUniform(static_cast<GLuint>(m_program_id), i, sizeof(buffer), &length, &size, &type, buffer);

		names.insert(buffer);
	}

	return names;
}

std::set<std::string> ShaderSet::get_attributes(void) const
{
	std::set<std::string> names;

	GLint count;

	glGetProgramiv(static_cast<GLuint>(m_program_id), GL_ACTIVE_ATTRIBUTES, &count);

	for (GLint i = 0; i < count; i++)
	{
		GLchar buffer[32];
		GLenum type;
		GLsizei length;
		GLint size;
		glGetActiveAttrib(static_cast<GLuint>(m_program_id), i, sizeof(buffer), &length, &size, &type, buffer);

		names.insert(buffer);
	}

	return names;
}

size_t ShaderSet::load_shader_from_file(const std::string& fname, const GLenum shaderType) const
{
	// Open file
	GLuint shaderID = 0;

	std::string shaderString;
	std::ifstream sourceFile(fname);

	// Source file loaded
	if (!sourceFile.is_open())
	{
		std::cout << "Unable to open file " << fname << std::endl;
		return shaderID;
	}

	// Get shader source
	shaderString.assign((std::istreambuf_iterator< char >(sourceFile)), std::istreambuf_iterator< char >());

	// Create shader ID
	shaderID = glCreateShader(shaderType);

	// Set shader source
	const GLchar* shaderSource = shaderString.c_str();
	glShaderSource(shaderID, 1, &shaderSource, nullptr);

	// Compile shader source
	glCompileShader(shaderID);

	// Check shader for errors
	GLint shaderCompiled = GL_FALSE;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderCompiled);

	if (shaderCompiled != GL_TRUE)
	{
		std::cout << "Unable to compile shader " << shaderID << std::endl;
		std::cout << "Source:" << std::endl;
		std::cout << shaderSource << std::endl;
		check_shader_log(shaderID, "compiling");
		glDeleteShader(shaderID);
		shaderID = 0;
	}

	return shaderID;
}

void ShaderSet::activate(void) const
{
	// Use shader
	glUseProgram(static_cast<GLuint>(m_program_id));

	// no error checking, this leads to error 1281: bad value
	// check_error("binding");
}

void ShaderSet::deactivate(void) const
{
	glUseProgram(0);
}

size_t ShaderSet::load_shaders(const std::string& file_vertex, const std::string& file_fragment, const std::string& file_geometry)
{
	if (m_program_id)
	{
		throw std::runtime_error("program already in use");
	}

	const float version = get_glsl_version();

	std::cout << "GLSL version: " << version << std::endl;

	if (version < 3.3f)
	{
		std::cout << "GLSL version not supported" << std::endl;
		// throw std::runtime_error("GLSL version not supported");
	}

	// Load vertex shader
	const GLuint vertexShader = static_cast<GLuint>(load_shader_from_file(file_vertex, GL_VERTEX_SHADER));

	if (vertexShader == 0)
	{
		return 0;
	}

	// Load geometry shader
	GLuint geometryShader = 0;

	if (!file_geometry.empty())
	{
		geometryShader = static_cast<GLuint>(load_shader_from_file(file_geometry, GL_GEOMETRY_SHADER));

		if (geometryShader == 0)
		{
			glDeleteShader(vertexShader);
			return 0;
		}
	}

	// Create fragment shader
	const GLuint fragmentShader = static_cast<GLuint>(load_shader_from_file(file_fragment, GL_FRAGMENT_SHADER));

	if (fragmentShader == 0)
	{
		glDeleteShader(vertexShader);

		if (geometryShader)
		{
			glDeleteShader(geometryShader);
		}
		return 0;
	}

	// Generate program
	GLuint mProgramID = glCreateProgram();

	if (mProgramID == 0)
	{
		std::cerr << "error creating programm" << std::endl;
		glDeleteShader(vertexShader);

		if (geometryShader)
		{
			glDeleteShader(geometryShader);
		}
		glDeleteShader(fragmentShader);
		return mProgramID;
	}

	glAttachShader(mProgramID, vertexShader);

	if (geometryShader)
	{
		glAttachShader(mProgramID, geometryShader);
	}
	glAttachShader(mProgramID, fragmentShader);
	glLinkProgram(mProgramID);

	// Check for errors
	GLint programSuccess = GL_TRUE;
	glGetProgramiv(mProgramID, GL_LINK_STATUS, &programSuccess);

	if (programSuccess != GL_TRUE)
	{
		std::cout << "Error linking program " << mProgramID << std::endl;
		check_program_log(mProgramID, "linking");
		glDetachShader(mProgramID, vertexShader);

		if (geometryShader)
		{
			glDetachShader(mProgramID, geometryShader);
		}
		glDetachShader(mProgramID, fragmentShader);
		glDeleteShader(vertexShader);

		if (geometryShader)
		{
			glDeleteShader(geometryShader);
		}
		glDeleteShader(fragmentShader);
		glDeleteProgram(mProgramID);
		mProgramID = 0;
		return mProgramID;
	}

	// Clean up excess shader references
	glDeleteShader(vertexShader);

	if (geometryShader)
	{
		glDeleteShader(geometryShader);
	}
	glDeleteShader(fragmentShader);

	m_program_id = mProgramID;
	return m_program_id;
}

size_t ShaderSet::load_compute_shader(const std::string& file_compute)
{
	if (m_program_id)
	{
		throw std::runtime_error("program already in use");
	}

	const float version = get_glsl_version();

	std::cout << "GLSL version: " << version << std::endl;

	if (version < 3.3f)
	{
		std::cout << "GLSL version not supported" << std::endl;
		// throw std::runtime_error("GLSL version not supported");
	}

	// Load vertex shader
	const GLuint computeShader = static_cast<GLuint>(load_shader_from_file(file_compute, GL_COMPUTE_SHADER));

	if (computeShader == 0)
	{
		return 0;
	}

	// Generate program
	GLuint mProgramID = glCreateProgram();

	if (mProgramID == 0)
	{
		std::cerr << "error creating programm" << std::endl;
		glDeleteShader(computeShader);
		return mProgramID;
	}

	glAttachShader(mProgramID, computeShader);
	glLinkProgram(mProgramID);

	// Check for errors
	GLint programSuccess = GL_TRUE;
	glGetProgramiv(mProgramID, GL_LINK_STATUS, &programSuccess);

	if (programSuccess != GL_TRUE)
	{
		std::cout << "Error linking program " << mProgramID << std::endl;
		check_program_log(mProgramID, "linking");
		glDetachShader(mProgramID, computeShader);
		glDeleteShader(computeShader);
		glDeleteProgram(mProgramID);
		mProgramID = 0;
		return mProgramID;
	}

	// Clean up excess shader references
	glDeleteShader(computeShader);

	m_program_id = mProgramID;
	return m_program_id;
}

GLint ShaderSet::get_location(const std::string& name) const
{
	activate();
	GLint loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());

	if (loc == -1)
	{
		throw std::runtime_error("failed locating uniform variable " + name);
	}
	return loc;
}

void ShaderSet::set_uniform(const std::string& name, const int val) const
{
	activate();
	int loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());

	if (loc != -1)
	{
		glUniform1i(loc, val);
	}
}

void ShaderSet::set_uniform(const std::string& name, const float val) const
{
	activate();
	int loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());

	if (loc != -1)
	{
		glUniform1f(loc, val);
	}
}

void ShaderSet::set_uniform(const std::string& name, const double val) const
{
	activate();
	int loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());

	if (loc != -1)
	{
		glUniform1d(loc, val);
	}
}

// void ShaderSet::set_uniform(const std::string& name, const std::complex<float>& val) const
// {
// activate();
// GLuint loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());
//
// if (loc != -1)
// {
// glUniform2f(loc, val.real(), val.imag());
// }
// }
//
// void ShaderSet::set_uniform(const std::string& name, const std::complex<double>& val) const
// {
// activate();
// GLuint loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());
//
// if (loc != -1)
// {
// glUniform2d(loc, val.real(), val.imag());
// }
// }

void ShaderSet::set_uniform(const std::string& name, const glm::mat4& val) const
{
	activate();
	int loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());

	if (loc != -1)
	{
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(val));
	}
}

void ShaderSet::set_uniform(const std::string& name, const glm::vec2& val) const
{
	activate();
	int loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());

	if (loc != -1)
	{
		glUniform2fv(loc, 1, glm::value_ptr(val));
	}
}

void ShaderSet::set_uniform(const std::string& name, const glm::vec3& val) const
{
	activate();
	int loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());

	if (loc != -1)
	{
		glUniform3fv(loc, 1, glm::value_ptr(val));
	}
}

void ShaderSet::set_uniform(const std::string& name, const glm::vec4& val) const
{
	activate();
	int loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());

	if (loc != -1)
	{
		glUniform4fv(loc, 1, glm::value_ptr(val));
	}
}

void ShaderSet::set_uniform(const std::string& name, const float val_1, const float val_2) const
{
	activate();
	int loc = glGetUniformLocation(static_cast<GLuint>(m_program_id), name.c_str());

	if (loc != -1)
	{
		glUniform2f(loc, val_1, val_2);
	}
}
