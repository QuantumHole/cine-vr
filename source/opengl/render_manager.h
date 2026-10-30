#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include "opengl/shape_set.h"
#include "opengl/texture.h"
#include "opengl/shader_set.h"
#include <cstddef>
#include <vector>

/** The RenderManager manages rendering all defined objects.
 * It is the single rendering instance for all OpenGL objects.
 */
class RenderManager
{
	private:
		typedef struct
		{
			bool active;
			size_t shapeset;
			std::vector<size_t> textures;
			size_t shader;
		}
		compound_t;

		std::vector<ShapeSet*> m_shapes;
		std::map<size_t, Texture*> m_textures;
		std::map<size_t, ShaderSet*> m_shaders;
		std::vector<compound_t> m_compounds;

		/** instance pointer */
		static RenderManager* m_instance;

		/** contructor. */
		explicit RenderManager(void);

	public:
		/** request the instance of this class.
		 * @return reference to the single instance.
		 */
		static RenderManager& instance(void);

		/** create a new shape set.
		 * @return ID of the shape set. This is the index into the ShapeSet vector.
		 */
		size_t create_shape_set(void);

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

		/** create a new texture.
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

		/** set activation state of compound.
		 * @param compound_id compound ID.
		 * @param active activation flag.
		 */
		void enable(const size_t compound_id,
		            const bool active);

		/** remove compound. All referenced entities are destroyed,
		 * if they are no longer used by other compounds.
		 * @param id compound ID.
		 */
		void delete_compound(const size_t id);

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

		/** draw all objects. */
		void draw(void) const;
};

#endif
