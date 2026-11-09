// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include "opengl/shape_set.h"
#include "opengl/texture.h"
#include "opengl/shader_set.h"
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

/** The RenderManager manages rendering all defined objects.
 * It is the single rendering instance for all OpenGL objects.
 */
class RenderManager
{
	public:
		typedef size_t RenderableId;

	private:
		typedef struct
		{
			bool is_group;
			bool active;
			bool deleted;
			bool draw_all;
			size_t shapeset;
			std::vector<size_t> textures;
			std::vector<size_t> children;
			size_t shader;
		}
		renderable_t;

		std::vector<std::unique_ptr<ShapeSet> > m_shapes;
		std::map<size_t, std::unique_ptr<Texture> > m_textures;
		std::map<size_t, std::unique_ptr<ShaderSet> > m_shaders;
		std::vector<renderable_t> m_renderables;
		size_t m_next_texture_id;

		/** instance pointer */
		static RenderManager* m_instance;

		/** contructor. */
		explicit RenderManager(void);
		void draw_renderable(const size_t id, const glm::mat4& transform, const float opacity) const;
		bool is_referenced_by_group(const size_t id) const;

	public:
		/** request the instance of this class.
		 * @return reference to the single instance.
		 */
		static RenderManager& instance(void);

		/** create a new shape set.
		 * @return ID of the shape set. This is the index into the ShapeSet vector.
		 */
		size_t create_shape_set(void);
		size_t load_svg(const std::string& file_name);
		ShapeSet& shape_set(const size_t id);

		/** create a new shape.
		 * @param set target set index.
		 * @param vertices vertices of the shape.
		 * @param indices definition of triangles from vertices.
		 * @param instances number of instances of this shape.
		 * @param type type of shapes to be constructed from vertices and indices.
		 * @return OpenGL ID of added shape.
		 */
		size_t add_shape(const size_t set,
		                 const std::vector<Vertex>& vertices,
		                 const std::vector<GLuint>& indices,
		                 const size_t instances = 1,
		                 const GLenum type = GL_TRIANGLES);

		/** create a manager-owned texture from pixel data.
		 * @param data image data for texture.
		 * @param size size of the texture in pixels.
		 * @param format texture data format.
		 * @param slot index of texture.
		 * @return OpenGL texture ID.
		 */
		size_t add_texture(const std::vector<uint8_t>& data,
		                   const glm::uvec2& size,
		                   const GLenum format,
		                   const GLuint slot);
		/** create an empty manager-owned texture. */
		size_t create_texture(void);
		Texture& texture(const size_t id);
		const Texture& texture(const size_t id) const;

		/** create a set of vertex, fragment, and (optional) geometry shaders.
		 * @param file_vertex file name of vertex shader.
		 * @param file_fragment file name of fragment shader.
		 * @param file_geometry optional file name of geometry shader.
		 * @return ID of the shader program.
		 */
		size_t add_shaders(const std::string& file_vertex,
		                   const std::string& file_fragment,
		                   const std::string& file_geometry = "");

		/** create compute shader.
		 * @param file_compute file name of compute shader.
		 * @return ID of the shader program.
		 */
		size_t add_shader_compute(const std::string& file_compute);

		/** get shader with ID.
		 * @param id shader program ID.
		 * @return reference to the shader.
		 */
		const ShaderSet& shader(const size_t id) const;
		ShaderSet& shader(const size_t id);

		/** create compound.
		 * Initially all created compounds are active for rendering.
		 * @param shape_id shape ID.
		 * @param textures vector of texture IDs. Some shaders may render shapes with multiple textures.
		 * @param shader_id shader ID.
		 * @return index of the created compound.
		 */
		size_t create_compound(const size_t shape_id,
		                       const std::vector<size_t>& textures,
		                       const size_t shader_id);

		/** create a renderable from one shape set. */
		RenderableId create_shape_set_renderable(const size_t shape_set_id,
		                                         const std::vector<size_t>& textures,
		                                         const size_t shader_id,
		                                         const bool draw_all = true);

		/** create a renderable containing one shape. */
		RenderableId create_shape_renderable(const std::vector<Vertex>& vertices,
		                                     const std::vector<GLuint>& indices,
		                                     const std::vector<size_t>& textures,
		                                     const size_t shader_id,
		                                     const size_t instances = 1,
	                                     const GLenum type = GL_TRIANGLES,
	                                     const bool draw_all = true);

		/** Create a renderable group. Children remain independently addressable. */
		RenderableId create_group(const std::vector<RenderableId>& children, const bool draw_all = true);

		/** set activation state of compound.
		 * @param compound_id compound ID.
		 * @param active activation flag.
		 */
		void enable(const size_t compound_id,
		            const bool active);
		void set_textures(const RenderableId id, const std::vector<size_t>& textures);

		/** invalidate a renderable handle; manager-owned resources remain available.
		 * @param id renderable ID.
		 */
		void delete_compound(const size_t id);
		void delete_renderable(const size_t id);

		/** set the transformation matrix for an instance of an object.
		 * @param set shape set index.
		 * @param index shape index in shape set.
		 * @param matrix transformation matrix.
		 * @param instance instance ID.
		 */
		void set_transform(const size_t set,
		                   const size_t index,
		                   const glm::mat4& matrix,
		                   const size_t instance = 0);
		void set_shape_set_transform(const size_t set, const glm::mat4& matrix);

		/** release manager-owned OpenGL resources while the context is current. */
		void clear(void);

		/** draw all objects. */
		void draw(void) const;
		void draw(const RenderableId id) const;
		void draw(const RenderableId id, const glm::mat4& transform, const float opacity = 1.0f) const;
};

#endif
