// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SHADER_H
#define SHADER_H

#include <GL/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <set>

class ShaderSet
{
	private:
		size_t m_program_id;

		float get_glsl_version(void) const;
		void check_program_log(const size_t program, const std::string& operation) const;
		void check_shader_log(const size_t shader, const std::string& operation) const;
		void check_error(const std::string& operation) const;
		size_t load_shader_from_file(const std::string& fname, const GLenum shaderType) const;

		unsigned int compile_shader(unsigned int shaderType, const char* shaderSource);

	public:
		explicit ShaderSet(void);
		~ShaderSet(void);

		size_t load_shaders(const std::string& file_vertex, const std::string& file_fragment, const std::string& file_geometry = "");
		size_t load_compute_shader(const std::string& file_compute);
		void activate(void) const;
		void deactivate(void) const;

		std::set<std::string> get_uniforms(void) const;
		std::set<std::string> get_attributes(void) const;

		size_t id(void) const;
		GLint get_location(const std::string& name) const;

		void set_uniform(const std::string& name, const int val) const;
		void set_uniform(const std::string& name, const float val) const;
		void set_uniform(const std::string& name, const double val) const;
		// void set_uniform(const std::string& name, const std::complex<float>& val) const;
		// void set_uniform(const std::string& name, const std::complex<double>& val) const;
		void set_uniform(const std::string& name, const glm::mat4& val) const;
		void set_uniform(const std::string& name, const glm::vec2& val) const;
		void set_uniform(const std::string& name, const glm::vec3& val) const;
		void set_uniform(const std::string& name, const glm::vec4& val) const;

		void set_uniform(const std::string& name, const float val_1, const float val_2) const;
};

#endif
