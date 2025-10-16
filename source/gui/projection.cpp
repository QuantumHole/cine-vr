// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <sstream>
#include <stdexcept>
#include "projection.h"

/*
 * projection    tiling        angle        zoom        stretch        follow HMD
 * ------------------------------------------------------------------------------
 * FLAT          MONO          -           screen       fix aspect     free movement
 *               LEFT_RIGHT                distance     ratio (*2)
 *               TOP_BOTTOM
 *
 * CYLINDER      MONO          section of  screen       fix aspect     free movement
 *               LEFT_RIGHT    cylinder    distance     ratio (*2)
 *               TOP_BOTTOM
 *
 * SPHERE        MONO          longitude   screen       -              geometry
 *               LEFT_RIGHT    section of  distance                    follows HMD
 *               TOP_BOTTOM    sphere
 *
 * CUBE_MAP      MONO          -           -            -              geometry
 *               LEFT_RIGHT                                            follows HMD
 *
 * FISHEYE       MONO          longitude   screen       -              geometry
 *               LEFT_RIGHT    section of  distance                    follows HMD
 *                             sphere
 */

static const glm::vec4 color_none(0.0f, 0.0f, 0.0f, 0.0f);
static const float radius = 10.0f;      // minimum distance to screen

Projection::Projection(void) :
	m_projection(PROJECTION_FLAT),
	m_tiling(TILE_MONO),
	m_angle(glm::pi<float>()),
	m_zoom(0.0f),
	m_aspect(1.0f),
	m_details(32),
	m_stretch(false),
	m_switch_eyes(false),
	m_mono(false),
	m_follow_hmd(false)
{
}

void Projection::set_projection(const video_projection_t p)
{
	m_projection = p;

	if ((p == PROJECTION_SPHERE) ||
	    (p == PROJECTION_FISHEYE) ||
	    (p == PROJECTION_CUBE_MAP))
	{
		m_follow_hmd = true;
	}
	else
	{
		m_follow_hmd = false;
	}
}

void Projection::set_tiling(const video_tiling_t t)
{
	m_tiling = t;
}

void Projection::set_angle(const float a)
{
	m_angle = a;
}

void Projection::set_zoom(const float z)
{
	m_zoom = z;
}

void Projection::set_aspect(const float a)
{
	m_aspect = a;
}

void Projection::set_stretch(const bool s)
{
	m_stretch = s;
}

void Projection::set_switch_eyes(const bool e)
{
	m_switch_eyes = e;
}

void Projection::set_mono(const bool m)
{
	m_mono = m;
}

Projection::video_projection_t Projection::projection(void) const
{
	return m_projection;
}

Projection::video_tiling_t Projection::tiling(void) const
{
	return m_tiling;
}

float Projection::angle(void) const
{
	return m_angle;
}

float Projection::zoom(void) const
{
	return m_zoom;
}

bool Projection::stretch(void) const
{
	return m_stretch;
}

bool Projection::follow_hmd(void) const
{
	return m_follow_hmd;
}

bool Projection::switch_eyes(void) const
{
	return m_switch_eyes;
}

bool Projection::mono(void) const
{
	return m_mono;
}

void Projection::map_cursor(glm::vec2& mouse) const
{
	switch (m_tiling)
	{
		case TILE_MONO:
		case TILE_CUBE_MAP_MONO:
			break;
		case TILE_LEFT_RIGHT:
		case TILE_CUBE_MAP_STEREO:

			if (mouse.x >= 0.5f)
			{
				mouse.x -= 0.5f;
			}
			break;
		case TILE_TOP_BOTTOM:

			if (mouse.y >= 0.5f)
			{
				mouse.y -= 0.5f;
			}
			break;
		default:
			break;
	}
}

/* scale factor for unit size with respect to size of projection */
glm::vec2 Projection::unit_scale(void) const
{
	switch (projection())
	{
		case Projection::PROJECTION_FLAT:
			/* height of 1, distance 1, scale for 1 degree viewing angle */
			return glm::vec2(1.0f / atanf(0.5f * m_aspect), m_aspect / atanf(0.5f));
		case Projection::PROJECTION_CYLINDER:
			/* scale to 1 degree viewing angle */
			return glm::vec2(1.0f / m_angle, m_aspect / m_angle);
		case Projection::PROJECTION_SPHERE:
			return glm::vec2(1.0f / m_angle, 1.0f / glm::pi<float>());
		case Projection::PROJECTION_CUBE_MAP:
			return glm::vec2(2.0f / (3.0f * glm::pi<float>()), 2.0f / (3.0f * glm::pi<float>()));
		case Projection::PROJECTION_FISHEYE:
			return glm::vec2(1.0f / m_angle, 1.0f / m_angle);
		default:
			throw std::runtime_error("invalid projection");
	}
}

std::pair<std::vector<Vertex>, std::vector<GLuint> > Projection::setup_projection_flat(void) const
{
	const float aspect = (m_stretch ? 1.0f : 0.5f) * m_aspect;
	const float hheight = 0.5f;
	const float hwidth = hheight * aspect;
	const float screen_distance = radius + m_zoom;

	// 0--1 1
	// | / /|
	// |/ / |
	// 2 2--3
	std::vector<Vertex> vertdata =
	{
		Vertex(glm::vec3(-hwidth,  hheight, -screen_distance), glm::vec3(0.0f, 0.0f, 1.0f), color_none, glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( hwidth,  hheight, -screen_distance), glm::vec3(0.0f, 0.0f, 1.0f), color_none, glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3(-hwidth, -hheight, -screen_distance), glm::vec3(0.0f, 0.0f, 1.0f), color_none, glm::vec2(0.0f, 1.0f)),
		Vertex(glm::vec3( hwidth, -hheight, -screen_distance), glm::vec3(0.0f, 0.0f, 1.0f), color_none, glm::vec2(1.0f, 1.0f))
	};

	std::vector<GLuint> indices = {
		0, 2, 1,
		2, 3, 1
	};

	return std::pair<std::vector<Vertex>, std::vector<GLuint> >(vertdata, indices);
}

std::pair<std::vector<Vertex>, std::vector<GLuint> > Projection::setup_projection_cylinder(void) const
{
	const float angle_start = -m_angle / 2;
	const float aspect = (m_stretch ? 1.0f : 0.5f) * m_aspect;
	const float height = m_angle * radius / aspect;

	std::vector<Vertex> vertdata;
	std::vector<GLuint> indices;

	for (size_t column = 0; column < m_details; column++)
	{
		float t1 = (static_cast<float>(column) / static_cast<float>(m_details));
		float t2 = (static_cast<float>(column + 1) / static_cast<float>(m_details));

		float x1 = sinf(angle_start + t1 * m_angle) * radius;
		float y1 = cosf(angle_start + t1 * m_angle) * radius;
		float x2 = sinf(angle_start + t2 * m_angle) * radius;
		float y2 = cosf(angle_start + t2 * m_angle) * radius;

		// .   2    _n
		// 1  /|  _/ |   _m
		// | / |_/   | _/ |
		// |/  2     n/   |
		// 1              m
		vertdata.push_back(Vertex(glm::vec3(x1,  height / 2, -m_zoom - y1), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(t1, 0)));
		vertdata.push_back(Vertex(glm::vec3(x2,  height / 2, -m_zoom - y2), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(t2, 0)));
		vertdata.push_back(Vertex(glm::vec3(x1, -height / 2, -m_zoom - y1), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(t1, 1)));
		vertdata.push_back(Vertex(glm::vec3(x2, -height / 2, -m_zoom - y2), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(t2, 1)));

		indices.push_back(static_cast<GLuint>(column * 4 + 0));
		indices.push_back(static_cast<GLuint>(column * 4 + 2));
		indices.push_back(static_cast<GLuint>(column * 4 + 1));
		indices.push_back(static_cast<GLuint>(column * 4 + 2));
		indices.push_back(static_cast<GLuint>(column * 4 + 3));
		indices.push_back(static_cast<GLuint>(column * 4 + 1));
	}

	return std::pair<std::vector<Vertex>, std::vector<GLuint> >(vertdata, indices);
}

std::pair<std::vector<Vertex>, std::vector<GLuint> > Projection::setup_projection_sphere(void) const
{
	const float angle_start = -m_angle / 2;

	std::vector<Vertex> vertdata;
	std::vector<GLuint> indices;

	for (size_t row = 0; row < m_details; row++)
	{
		for (size_t column = 0; column < m_details; column++)
		{
			const float tx1 = static_cast<float>(column) / static_cast<float>(m_details);
			const float tx2 = static_cast<float>(column + 1) / static_cast<float>(m_details);
			const float ty1 = static_cast<float>(row) / static_cast<float>(m_details);
			const float ty2 = static_cast<float>(row + 1) / static_cast<float>(m_details);

			const float y_sin1 = sinf(ty1 * glm::pi<float>()); // distance from vertical middle axis
			const float y_sin2 = sinf(ty2 * glm::pi<float>());

			float z1 = cosf(angle_start + tx1 * m_angle) * radius;           // z-position of vertices (front/back)
			float z2 = cosf(angle_start + tx2 * m_angle) * radius;
			float z3 = z1;
			float z4 = z2;

			z1 *= y_sin1;
			z2 *= y_sin1;
			z3 *= y_sin2;
			z4 *= y_sin2;

			float x1 = sinf(angle_start + tx1 * m_angle) * radius;
			float x2 = sinf(angle_start + tx2 * m_angle) * radius;
			float x3 = x1;
			float x4 = x2;

			x1 *= y_sin1;
			x2 *= y_sin1;
			x3 *= y_sin2;
			x4 *= y_sin2;

			const float y1 = cosf(ty1 * glm::pi<float>()) * radius;
			const float y2 = y1;
			const float y3 = cosf(ty2 * glm::pi<float>()) * radius;
			const float y4 = y3;

			// 0--1 1
			// | / /|
			// |/ / |
			// 2 2--3
			vertdata.push_back(Vertex(glm::vec3(x1, y1, -z1 - m_zoom), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(tx1, ty1)));
			vertdata.push_back(Vertex(glm::vec3(x2, y2, -z2 - m_zoom), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(tx2, ty1)));
			vertdata.push_back(Vertex(glm::vec3(x3, y3, -z3 - m_zoom), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(tx1, ty2)));
			vertdata.push_back(Vertex(glm::vec3(x4, y4, -z4 - m_zoom), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(tx2, ty2)));

			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 0));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 2));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 1));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 2));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 3));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 1));
		}
	}

	return std::pair<std::vector<Vertex>, std::vector<GLuint> >(vertdata, indices);
}

std::pair<std::vector<Vertex>, std::vector<GLuint> > Projection::setup_projection_fisheye(void) const
{
	const float angle_max = m_angle / 2.0f;
	const float eps = std::numeric_limits<float>::epsilon();

	std::vector<Vertex> vertdata;
	std::vector<GLuint> indices;

	for (size_t row = 0; row < m_details; row++)
	{
		for (size_t column = 0; column < m_details; column++)
		{
			/* x: left-right
			 * y: up-down
			 * z: forward-backward
			 */

			/* (x,y) coordinates of four tile corners */
			const float tx1 = static_cast<float>(column) / static_cast<float>(m_details) - 0.5f;
			const float tx2 = static_cast<float>(column + 1) / static_cast<float>(m_details) - 0.5f;
			const float ty1 = static_cast<float>(row) / static_cast<float>(m_details) - 0.5f;
			const float ty2 = static_cast<float>(row + 1) / static_cast<float>(m_details) - 0.5f;

			/* distances from image center */
			const float tr1 = sqrtf(tx1 * tx1 + ty1 * ty1);
			const float tr2 = sqrtf(tx2 * tx2 + ty1 * ty1);
			const float tr3 = sqrtf(tx1 * tx1 + ty2 * ty2);
			const float tr4 = sqrtf(tx2 * tx2 + ty2 * ty2);

			/* note: projection angles from image center
			 * r = 2 * f * sin(theta/2)
			 * f = r / (2 * sin(theta/2))
			 * . = 0.5 / (2 * sin(theta_max/2)))
			 * . = 1 / (4 * sin(theta_max/2)))
			 * theta = 2 * arcsin(r / (2*f))
			 * .     = 2 * arcsin(r / (2*(1 / (4 * sin(theta_max/2)))))
			 * .     = 2 * arcsin(r / (2 / (4 * sin(theta_max/2))))
			 * .     = 2 * arcsin(r / (1 / (2 * sin(theta_max/2))))
			 * .     = 2 * arcsin(r * 2 * sin(theta_max/2))
			 */
			const float focus = 2 * sinf(0.5f * angle_max);
			const float angle_1 = 2 * asinf(std::min(tr1 * focus, 1.0f));
			const float angle_2 = 2 * asinf(std::min(tr2 * focus, 1.0f));
			const float angle_3 = 2 * asinf(std::min(tr3 * focus, 1.0f));
			const float angle_4 = 2 * asinf(std::min(tr4 * focus, 1.0f));

			const float z1 = cosf(angle_1) * radius;
			const float z2 = cosf(angle_2) * radius;
			const float z3 = cosf(angle_3) * radius;
			const float z4 = cosf(angle_4) * radius;

			const float r1 = sinf(angle_1) * radius;
			const float r2 = sinf(angle_2) * radius;
			const float r3 = sinf(angle_3) * radius;
			const float r4 = sinf(angle_4) * radius;

			// tan(azi) = dy1 / dx1
			// tan(azi) = y1 / x1    ==>   y1 = tan(azi) * x1 = x1 * dy1 / dx1
			// cos(azi) = dx1 / dr1
			// cos(azi) = x1 / r1    ==>   x1 = r1 * cos(azi) = r1 * dx1 /dr1

			const float x1 = ((tr1 < eps) ? 0.0f : (r1 * tx1 / tr1));
			const float x2 = ((tr2 < eps) ? 0.0f : (r2 * tx2 / tr2));
			const float x3 = ((tr3 < eps) ? 0.0f : (r3 * tx1 / tr3));
			const float x4 = ((tr4 < eps) ? 0.0f : (r4 * tx2 / tr4));

			/* negative sign for right handed coordinate system */
			const float y1 = ((angle_1 < eps) ? 0.0f : ((fabsf(tx1) < eps) ? (-r1 * ty1 / fabsf(ty1)) : (-x1 * ty1 / tx1)));
			const float y2 = ((angle_2 < eps) ? 0.0f : ((fabsf(tx2) < eps) ? (-r2 * ty1 / fabsf(ty1)) : (-x2 * ty1 / tx2)));
			const float y3 = ((angle_3 < eps) ? 0.0f : ((fabsf(tx1) < eps) ? (-r3 * ty2 / fabsf(ty2)) : (-x3 * ty2 / tx1)));
			const float y4 = ((angle_4 < eps) ? 0.0f : ((fabsf(tx2) < eps) ? (-r4 * ty2 / fabsf(ty2)) : (-x4 * ty2 / tx2)));

			// 0--1 1
			// | / /|
			// |/ / |
			// 2 2--3
			vertdata.push_back(Vertex(glm::vec3(x1, y1, -z1 - m_zoom), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f + tx1, ty1 + 0.5f)));
			vertdata.push_back(Vertex(glm::vec3(x2, y2, -z2 - m_zoom), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f + tx2, ty1 + 0.5f)));
			vertdata.push_back(Vertex(glm::vec3(x3, y3, -z3 - m_zoom), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f + tx1, ty2 + 0.5f)));
			vertdata.push_back(Vertex(glm::vec3(x4, y4, -z4 - m_zoom), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f + tx2, ty2 + 0.5f)));

			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 0));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 2));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 1));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 2));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 3));
			indices.push_back(static_cast<GLuint>((row * m_details + column) * 4 + 1));
		}
	}

	return std::pair<std::vector<Vertex>, std::vector<GLuint> >(vertdata, indices);
}

std::pair<std::vector<Vertex>, std::vector<GLuint> > Projection::setup_projection_cubemap_mono(void) const
{
	std::vector<Vertex> vertdata =
	{
		/* left side */
		Vertex(glm::vec3(-radius,  radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(1.0f / 3.0f, 0.0f)),
		Vertex(glm::vec3(-radius,  radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.0f / 3.0f, 0.0f)),
		Vertex(glm::vec3(-radius, -radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.0f / 3.0f, 0.5f)),
		Vertex(glm::vec3(-radius, -radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(1.0f / 3.0f, 0.5f)),

		/* front view */
		Vertex(glm::vec3(radius,  radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(2.0f / 3.0f, 0.0f)),
		Vertex(glm::vec3(radius, -radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(2.0f / 3.0f, 0.5f)),

		/* right side */
		Vertex(glm::vec3(radius,  radius, radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(3.0f / 3.0f, 0.0f)),
		Vertex(glm::vec3(radius, -radius, radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(3.0f / 3.0f, 0.5f)),

		/* top */
		Vertex(glm::vec3( radius, radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(3.0f / 3.0f, 0.5f)),
		Vertex(glm::vec3( radius, radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(2.0f / 3.0f, 0.5f)),
		Vertex(glm::vec3(-radius, radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(2.0f / 3.0f, 1.0f)),
		Vertex(glm::vec3(-radius, radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(3.0f / 3.0f, 1.0f)),

		/* back view */
		Vertex(glm::vec3( radius, -radius, radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(1.0f / 3.0f, 0.5f)),
		Vertex(glm::vec3(-radius, -radius, radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(1.0f / 3.0f, 1.0f)),

		/* bottom */
		Vertex(glm::vec3( radius, -radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.0f / 3.0f, 0.5f)),
		Vertex(glm::vec3(-radius, -radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.0f / 3.0f, 1.0f))
	};

	std::vector<GLuint> indices = {
		0, 1, 2,
		2, 3, 0,
		4, 0, 3,
		3, 5, 4,
		4, 7, 6,
		7, 4, 5,
		8, 9, 10,
		10, 11, 8,
		12, 13, 10,
		10, 9, 12,
		14, 13, 12,
		13, 14, 15
	};

	return std::pair<std::vector<Vertex>, std::vector<GLuint> >(vertdata, indices);
}

std::pair<std::vector<Vertex>, std::vector<GLuint> > Projection::setup_projection_cubemap_stereo(void) const
{
	std::vector<Vertex> vertdata =
	{
		Vertex(glm::vec3( radius,  radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.0f, 1.0f / 3.0f)),
		Vertex(glm::vec3( radius, -radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f, 1.0f / 3.0f)),
		Vertex(glm::vec3( radius, -radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f, 0.0f / 3.0f)),
		Vertex(glm::vec3( radius,  radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.0f, 0.0f / 3.0f)),
		Vertex(glm::vec3(-radius,  radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.0f, 2.0f / 3.0f)),
		Vertex(glm::vec3(-radius, -radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f, 2.0f / 3.0f)),
		Vertex(glm::vec3(-radius,  radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.0f, 3.0f / 3.0f)),
		Vertex(glm::vec3(-radius, -radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f, 3.0f / 3.0f)),
		Vertex(glm::vec3(-radius,  radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(1.0f, 0.0f / 3.0f)),
		Vertex(glm::vec3( radius,  radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f, 0.0f / 3.0f)),
		Vertex(glm::vec3( radius,  radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f, 1.0f / 3.0f)),
		Vertex(glm::vec3(-radius,  radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(1.0f, 1.0f / 3.0f)),
		Vertex(glm::vec3( radius, -radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f, 2.0f / 3.0f)),
		Vertex(glm::vec3(-radius, -radius,  radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(1.0f, 2.0f / 3.0f)),
		Vertex(glm::vec3( radius, -radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(0.5f, 3.0f / 3.0f)),
		Vertex(glm::vec3(-radius, -radius, -radius), glm::vec3(0.0f, 0.0f, 0.0f), color_none, glm::vec2(1.0f, 3.0f / 3.0f)),
	};

	std::vector<GLuint> indices = {
		0, 1, 2,
		2, 3, 0,
		1, 0, 4,
		4, 5, 1,
		6, 7, 5,
		5, 4, 6,
		8, 9, 10,
		10, 11, 8,
		11, 10, 12,
		12, 13, 11,
		13, 12, 14,
		14, 15, 13
	};

	return std::pair<std::vector<Vertex>, std::vector<GLuint> >(vertdata, indices);
}

std::pair<std::vector<Vertex>, std::vector<GLuint> > Projection::setup_projection(void) const
{
	std::pair<std::vector<Vertex>, std::vector<GLuint> > vertdata;

	switch (projection())
	{
		case Projection::PROJECTION_FLAT:
			vertdata = setup_projection_flat();
			break;
		case Projection::PROJECTION_CYLINDER:
			vertdata = setup_projection_cylinder();
			break;
		case Projection::PROJECTION_SPHERE:
			vertdata = setup_projection_sphere();
			break;
		case Projection::PROJECTION_CUBE_MAP:

			if (tiling() == Projection::TILE_CUBE_MAP_MONO)
			{
				vertdata = setup_projection_cubemap_mono();
			}
			else if (tiling() == Projection::TILE_CUBE_MAP_STEREO)
			{
				vertdata = setup_projection_cubemap_stereo();
			}
			break;
		case Projection::PROJECTION_FISHEYE:
			vertdata = setup_projection_fisheye();
			break;
		default:
			throw std::runtime_error("invalid projection");
	}
	return vertdata;
}
