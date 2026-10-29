// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EBO_H
#define EBO_H

#include <GL/gl.h>
#include <openvr/openvr.h>
#include <vector>

/** represents an Element Buffer Object (EBO).
 * This buffer holds a list of indices into a previously defined vertex buffer object (VBO).
 */
class EBO
{
	private:
		GLuint m_id;
		GLuint m_num_indices;
		GLenum m_index_format;

	public:
		/** default constructor. */
		EBO(void);

		/** ID of this buffer object.
		 * This is identical to the OpenGL ID.
		 * @return ID if the buffer object.
		 */
		GLuint id(void) const;

		/** number of indices in the related buffer.
		 * @return number of indices.
		 */
		GLuint num_indices(void) const;

		/** format of the index buffer.
		 * Implemented values are GL_UNSIGNED_INT and GL_UNSIGNED_SHORT.
		 * @return format of the index buffer.
		 */
		GLenum index_format(void) const;

		/** initialize the buffer with given indeces.
		 * @param indices vector of indices for the buffer.
		 */
		void init(const std::vector<GLuint>& indices);

		/** initialize the buffer with an OpenVR RenderModel.
		 * @param model the render model with all the indices.
		 */
		void init(const vr::RenderModel_t& model);

		/** activate the buffer object. */
		void bind(void) const;

		/** deactivate the buffer object. */
		void unbind(void) const;

		/** delete the buffer object from OpenGL memory. */
		void remove();
};

#endif
