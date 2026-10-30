#include "render_manager.h"
#include "shader_set.h"
#include "texture.h"
#include "vertex.h"

#include <GL/gl.h>
#include <map>
#include <stdexcept>
#include <algorithm>

RenderManager* RenderManager::m_instance = nullptr;

RenderManager::RenderManager(void) :
	m_shapes(),
	m_textures(),
	m_shaders(),
	m_compounds()
{
}

RenderManager& RenderManager::instance(void)
{
	if (m_instance == nullptr)
	{
		m_instance = new RenderManager();
	}
	return *m_instance;
}

size_t RenderManager::create_shape_set(void)
{
	ShapeSet* newShapeSet = new ShapeSet();

	m_shapes.push_back(newShapeSet);
	return m_shapes.size() - 1;
}

size_t RenderManager::add_shape(const size_t set,
                                const std::vector<Vertex>& vertices,
                                const std::vector<GLuint>& indices,
                                const size_t instances,
                                const GLenum type)
{
	return m_shapes.at(set)->add(vertices, indices, type, instances);
}

size_t RenderManager::add_texture(const std::vector<uint8_t>& data,
                                  const glm::uvec2& size,
                                  const GLenum format,
                                  const GLuint slot)
{
	Texture* t = new Texture();

	t->init_data(data, size, format, slot);
	m_textures.insert(std::pair<size_t, Texture*>(t->id(), t));
	return t->id();
}

size_t RenderManager::add_shaders(const std::string& file_vertex,
                                  const std::string& file_fragment,
                                  const std::string& file_geometry)
{
	ShaderSet* s = new ShaderSet();

	s->load_shaders(file_vertex, file_fragment, file_geometry);
	m_shaders[s->id()] = s;
	return s->id();
}

size_t RenderManager::add_shader_compute(const std::string& file_compute)
{
	ShaderSet* s = new ShaderSet();

	s->load_compute_shader(file_compute);
	m_shaders[s->id()] = s;
	return s->id();
}

const ShaderSet& RenderManager::shader(const size_t id) const
{
	std::map<size_t, ShaderSet*>::const_iterator iter = m_shaders.find(id);

	if (iter == m_shaders.end())
	{
		throw std::runtime_error("invalid shader ID");
	}
	return *(iter->second);
}

size_t RenderManager::create_compound(const size_t shape_id,
                                      const std::vector<size_t>& textures,
                                      const size_t shader_id)
{
	compound_t c = {true, shape_id, textures, shader_id};
	const size_t n = m_compounds.size();

	m_compounds.push_back(c);
	return n;
}

void RenderManager::delete_compound(const size_t id)
{
	if (id >= m_compounds.size())
	{
		throw std::runtime_error("invalid compound ID");
	}

	compound_t& c = m_compounds.at(id);
	c.active = false;

	// Prüfen, ob ShapeSet noch von anderen Compounds verwendet wird
	bool shape_used = false;
	for (size_t i = 0; i < m_compounds.size(); ++i)
	{
		if (i == id)
		{
			continue;
		}

		if (m_compounds[i].shapeset == c.shapeset)
		{
			shape_used = true;
			break;
		}
	}

	if (!shape_used)
	{
		if ((c.shapeset >= m_shapes.size()) || (m_shapes[c.shapeset] == nullptr))
		{
			throw std::runtime_error("invalid shape");
		}
		delete m_shapes[c.shapeset];
		m_shapes[c.shapeset] = nullptr; // markiert als gelöscht
	}

	// Prüfen, ob Textures noch von anderen Compounds verwendet werden
	for (size_t texID : c.textures)
	{
		bool texture_used = false;
		for (const auto& comp : m_compounds)
		{
			if (std::find(comp.textures.begin(), comp.textures.end(), texID) != comp.textures.end())
			{
				texture_used = true;
				break;
			}
		}

		if (!texture_used)
		{
			auto iter = m_textures.find(texID);

			if (iter == m_textures.end())
			{
				throw std::runtime_error("invalid texture");
			}
			delete iter->second;
			m_textures.erase(iter);
		}
	}

	// Prüfen, ob Shader noch von anderen Compounds verwendet wird
	bool shader_used = false;
	for (const auto& comp : m_compounds)
	{
		if ((&comp != &c) && (comp.shader == c.shader))
		{
			shader_used = true;
			break;
		}
	}

	if (!shader_used)
	{
		auto iter = m_shaders.find(c.shader);

		if (iter == m_shaders.end())
		{
			throw std::runtime_error("invalid shader");
		}
		delete iter->second;
		m_shaders.erase(iter);
	}
}

void RenderManager::enable(const size_t compound_id, const bool active)
{
	if (compound_id >= m_compounds.size())
	{
		throw std::runtime_error("invalid compound ID");
	}
	m_compounds[compound_id].active = active;
}

void RenderManager::set_transform(const size_t set,
                                  const size_t index,
                                  const glm::mat4& matrix,
                                  const size_t instance)
{
	if ((set >= m_shapes.size()) || (m_shapes[set] == nullptr))
	{
		throw std::runtime_error("invalid shape set");
	}

	m_shapes[set]->set_transform(index, matrix, instance);
}

void RenderManager::draw(void) const
{
	for (const auto& c : m_compounds)
	{
		if (!c.active)
		{
			continue;
		}

		if ((c.shapeset >= m_shapes.size()) || (m_shapes[c.shapeset] == nullptr))
		{
			continue;
		}

		// Shader binden
		auto shaderIt = m_shaders.find(c.shader);

		if (shaderIt == m_shaders.end())
		{
			continue;
		}
		shaderIt->second->activate();

		// Textures binden
		for (size_t texID : c.textures)
		{
			auto texIt = m_textures.find(texID);

			if (texIt != m_textures.end())
			{
				texIt->second->bind();
			}
		}

		// Shape zeichnen
		m_shapes[c.shapeset]->draw();

		// Optional: Shader und Textures wieder lösen
		for (size_t texID : c.textures)
		{
			auto texIt = m_textures.find(texID);

			if (texIt != m_textures.end())
			{
				texIt->second->unbind();
			}
		}
		shaderIt->second->deactivate();
	}
}
