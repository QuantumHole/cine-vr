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
	m_renderables(),
	m_next_texture_id(1)
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
	const size_t id = m_next_texture_id++;
	m_textures.insert(std::make_pair(id, texture_t{t, nullptr, slot}));
	return id;
}

size_t RenderManager::add_external_texture(TextureSource& source,
                                           const GLuint slot)
{
	const size_t id = m_next_texture_id++;
	m_textures.insert(std::make_pair(id, texture_t{nullptr, &source, slot}));
	return id;
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

ShaderSet& RenderManager::shader(const size_t id)
{
	std::map<size_t, ShaderSet*>::iterator iter = m_shaders.find(id);
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
	return create_shape_set_renderable(shape_id, textures, shader_id);
}

size_t RenderManager::create_shape_set_renderable(const size_t shape_set_id,
                                                  const std::vector<size_t>& textures,
                                                  const size_t shader_id)
{
	if ((shape_set_id >= m_shapes.size()) || (m_shapes[shape_set_id] == nullptr))
	{
		throw std::runtime_error("invalid shape set");
	}
	if (m_shaders.find(shader_id) == m_shaders.end())
	{
		throw std::runtime_error("invalid shader ID");
	}
	for (size_t texture_id : textures)
	{
		if (m_textures.find(texture_id) == m_textures.end())
		{
			throw std::runtime_error("invalid texture ID");
		}
	}
	renderable_t renderable = {false, true, false, shape_set_id, textures, std::vector<size_t>(), shader_id};
	const size_t id = m_renderables.size();
	m_renderables.push_back(renderable);
	return id;
}

size_t RenderManager::create_shape_renderable(const std::vector<Vertex>& vertices,
                                              const std::vector<GLuint>& indices,
                                              const std::vector<size_t>& textures,
                                              const size_t shader_id,
                                              const size_t instances,
                                              const GLenum type)
{
	if (m_shaders.find(shader_id) == m_shaders.end())
	{
		throw std::runtime_error("invalid shader ID");
	}
	for (size_t texture_id : textures)
	{
		if (m_textures.find(texture_id) == m_textures.end())
		{
			throw std::runtime_error("invalid texture ID");
		}
	}
	const size_t shape_set_id = create_shape_set();
	add_shape(shape_set_id, vertices, indices, instances, type);
	return create_shape_set_renderable(shape_set_id, textures, shader_id);
}

size_t RenderManager::create_group(const std::vector<size_t>& children)
{
	for (auto child_iter = children.begin(); child_iter != children.end(); ++child_iter)
	{
		const size_t child = *child_iter;
		if ((child >= m_renderables.size()) || m_renderables[child].deleted)
		{
			throw std::runtime_error("invalid child renderable ID");
		}
		if (is_referenced_by_group(child) ||
		    (std::find(children.begin(), child_iter, child) != child_iter))
		{
			throw std::runtime_error("renderable already has a parent group");
		}
	}
	renderable_t group = {true, true, false, 0, std::vector<size_t>(), children, 0};
	const size_t id = m_renderables.size();
	m_renderables.push_back(group);
	return id;
}

bool RenderManager::is_referenced_by_group(const size_t id) const
{
	for (const renderable_t& renderable : m_renderables)
	{
		if (!renderable.deleted && renderable.is_group &&
		    (std::find(renderable.children.begin(), renderable.children.end(), id) != renderable.children.end()))
		{
			return true;
		}
	}
	return false;
}

void RenderManager::release_unreferenced_resources(const renderable_t& renderable)
{
	bool shape_set_used = false;
	bool shader_used = false;
	for (const renderable_t& other : m_renderables)
	{
		if (other.deleted || other.is_group)
		{
			continue;
		}
		shape_set_used = shape_set_used || (other.shapeset == renderable.shapeset);
		shader_used = shader_used || (other.shader == renderable.shader);
	}

	if (!shape_set_used && (renderable.shapeset < m_shapes.size()) && (m_shapes[renderable.shapeset] != nullptr))
	{
		delete m_shapes[renderable.shapeset];
		m_shapes[renderable.shapeset] = nullptr;
	}
	if (!shader_used)
	{
		auto shader_iter = m_shaders.find(renderable.shader);
		if (shader_iter != m_shaders.end())
		{
			delete shader_iter->second;
			m_shaders.erase(shader_iter);
		}
	}

	for (size_t texture_id : renderable.textures)
	{
		bool texture_used = false;
		for (const renderable_t& other : m_renderables)
		{
			if (!other.deleted && !other.is_group &&
			    (std::find(other.textures.begin(), other.textures.end(), texture_id) != other.textures.end()))
			{
				texture_used = true;
				break;
			}
		}
		if (!texture_used)
		{
			auto texture_iter = m_textures.find(texture_id);
			if ((texture_iter != m_textures.end()) && (texture_iter->second.owned != nullptr))
			{
				texture_iter->second.owned->remove();
				delete texture_iter->second.owned;
				m_textures.erase(texture_iter);
			}
		}
	}
}

void RenderManager::delete_compound(const size_t id)
{
	delete_renderable(id);
}

void RenderManager::delete_renderable(const size_t id)
{
	if ((id >= m_renderables.size()) || m_renderables[id].deleted)
	{
		throw std::runtime_error("invalid renderable ID");
	}
	if (is_referenced_by_group(id))
	{
		throw std::runtime_error("renderable is still used by a group");
	}
	renderable_t& renderable = m_renderables[id];
	renderable.active = false;
	renderable.deleted = true;
	if (!renderable.is_group)
	{
		release_unreferenced_resources(renderable);
	}
}

void RenderManager::enable(const size_t compound_id, const bool active)
{
	if ((compound_id >= m_renderables.size()) || m_renderables[compound_id].deleted)
	{
		throw std::runtime_error("invalid renderable ID");
	}
	m_renderables[compound_id].active = active;
}

void RenderManager::set_textures(const RenderableId id, const std::vector<size_t>& textures)
{
	if ((id >= m_renderables.size()) || m_renderables[id].deleted || m_renderables[id].is_group)
	{
		throw std::runtime_error("invalid renderable ID");
	}
	for (size_t texture_id : textures)
	{
		if (m_textures.find(texture_id) == m_textures.end())
		{
			throw std::runtime_error("invalid texture ID");
		}
	}
	const std::vector<size_t> old_textures = m_renderables[id].textures;
	m_renderables[id].textures = textures;
	for (size_t texture_id : old_textures)
	{
		if (std::find(textures.begin(), textures.end(), texture_id) != textures.end())
		{
			continue;
		}
		bool texture_used = false;
		for (const renderable_t& other : m_renderables)
		{
			if (!other.deleted && !other.is_group &&
			    (std::find(other.textures.begin(), other.textures.end(), texture_id) != other.textures.end()))
			{
				texture_used = true;
				break;
			}
		}
		if (!texture_used)
		{
			auto texture_iter = m_textures.find(texture_id);
			if ((texture_iter != m_textures.end()) && (texture_iter->second.owned != nullptr))
			{
				texture_iter->second.owned->remove();
				delete texture_iter->second.owned;
				m_textures.erase(texture_iter);
			}
		}
	}
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

void RenderManager::clear(void)
{
	for (ShapeSet* shape_set : m_shapes)
	{
		delete shape_set;
	}
	m_shapes.clear();
	for (auto& texture_entry : m_textures)
	{
		if (texture_entry.second.owned != nullptr)
		{
			texture_entry.second.owned->remove();
			delete texture_entry.second.owned;
		}
	}
	m_textures.clear();
	for (auto& shader_entry : m_shaders)
	{
		delete shader_entry.second;
	}
	m_shaders.clear();
	m_renderables.clear();
}

void RenderManager::draw(void) const
{
	for (size_t id = 0; id < m_renderables.size(); ++id)
	{
		if (!is_referenced_by_group(id))
		{
			draw_renderable(id);
		}
	}
}

void RenderManager::draw_renderable(const size_t id) const
{
	const renderable_t& renderable = m_renderables[id];
	if (renderable.deleted || !renderable.active)
	{
		return;
	}
	if (renderable.is_group)
	{
		for (size_t child : renderable.children)
		{
			draw_renderable(child);
		}
		return;
	}
	if ((renderable.shapeset >= m_shapes.size()) || (m_shapes[renderable.shapeset] == nullptr))
	{
		return;
	}
	auto shader_iter = m_shaders.find(renderable.shader);
	if (shader_iter == m_shaders.end())
	{
		return;
	}
	shader_iter->second->activate();
	for (size_t texture_id : renderable.textures)
	{
		auto texture_iter = m_textures.find(texture_id);
		if (texture_iter == m_textures.end())
		{
			continue;
		}
		if (texture_iter->second.owned != nullptr)
		{
			texture_iter->second.owned->bind();
		}
		else
		{
			const GLuint texture_name = texture_iter->second.external->texture_id();
			glActiveTexture(GL_TEXTURE0 + texture_iter->second.slot);
			glBindTexture(GL_TEXTURE_2D, texture_name);
		}
	}
	m_shapes[renderable.shapeset]->draw();
	for (size_t texture_id : renderable.textures)
	{
		auto texture_iter = m_textures.find(texture_id);
		if (texture_iter != m_textures.end())
		{
			glActiveTexture(GL_TEXTURE0 + texture_iter->second.slot);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	shader_iter->second->deactivate();
}
