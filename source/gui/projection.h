// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROJECTION_H
#define PROJECTION_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "opengl/vertex.h"

class Projection
{
	public:
		typedef enum
		{
			TILE_MONO,
			TILE_LEFT_RIGHT,
			TILE_TOP_BOTTOM,
			TILE_CUBE_MAP_MONO,
			TILE_CUBE_MAP_STEREO,
			NUM_TILINGS
		}
		video_tiling_t;

		typedef enum
		{
			PROJECTION_FLAT,
			PROJECTION_CYLINDER,     /* a.k.a. curved plane */
			PROJECTION_SPHERE,
			PROJECTION_FISHEYE,
			PROJECTION_CUBE_MAP,
			NUM_PROJECTIONS
		}
		video_projection_t;

		Projection(void);

		void set_projection(const video_projection_t p);
		void set_tiling(const video_tiling_t t);
		void set_angle(const float a); /* angle given in radians */
		void set_zoom(const float z);
		void set_aspect(const float a);
		void set_stretch(const bool s);
		void set_switch_eyes(const bool e);
		void set_mono(const bool m);

		video_projection_t projection(void) const;
		video_tiling_t tiling(void) const;
		float angle(void) const; /* angle given in radians */
		float zoom(void) const;
		bool stretch(void) const;
		bool switch_eyes(void) const;
		bool mono(void) const;

		bool follow_hmd(void) const;
		void map_cursor(glm::vec2& mouse) const;
		glm::vec2 unit_scale(void) const;

		std::pair<std::vector<Vertex>, std::vector<GLuint> > setup_projection(void) const;

	private:
		video_projection_t m_projection;
		video_tiling_t m_tiling;
		float m_angle;
		float m_zoom;
		float m_aspect;
		size_t m_details;
		bool m_stretch;
		bool m_switch_eyes;
		bool m_mono;
		bool m_follow_hmd;   /* configured automatically with projection mode */

		std::pair<std::vector<Vertex>, std::vector<GLuint> > setup_projection_flat(void) const;
		std::pair<std::vector<Vertex>, std::vector<GLuint> > setup_projection_cylinder(void) const;
		std::pair<std::vector<Vertex>, std::vector<GLuint> > setup_projection_sphere(void) const;
		std::pair<std::vector<Vertex>, std::vector<GLuint> > setup_projection_fisheye(void) const;
		std::pair<std::vector<Vertex>, std::vector<GLuint> > setup_projection_cubemap_mono(void) const;
		std::pair<std::vector<Vertex>, std::vector<GLuint> > setup_projection_cubemap_stereo(void) const;
};

#endif
