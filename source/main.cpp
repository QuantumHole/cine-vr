// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <openvr.h>

// #define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "opengl/shader_set.h"
#include "opengl/framebuffer.h"
#include "openvr_interface.h"
#include "enum_iterator.h"

typedef EnumIterator<vr::Hmd_Eye, vr::Eye_Left, vr::Eye_Right> Eyes;

typedef struct
{
	glm::vec3 origin;
	glm::vec3 direction;
} ray_t;

typedef struct
{
	bool hit;
	glm::vec3 global;
	glm::vec2 local;    // texture coordinates
} intersection_t;

// Render a rectangle (centered at origin, size 1x1 in XY plane)
struct Mesh
{
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	int idxCount = 0;
};

static bool g_Running = true;
static GLFWwindow* g_Window = nullptr;
static OpenVRInterface g_vr;

static Mesh g_RectMesh;
static Mesh g_LineMesh;
static Mesh g_PointMesh;
static ShaderSet g_shaders;

static std::vector<Framebuffer> g_framebuffer(Eyes::size());

static void CreateRectMesh()
{
	// positions: rectangle in XY plane centered at 0, z=0
	float hw = 0.5f;
	float hh = 0.5f;
	struct V
	{
		float x, y, z;
		float r, g, b;
	};
	std::vector<V> verts = {
		{-hw, -hh, 0.0f, 0.8f, 0.2f, 0.2f},
		{ hw, -hh, 0.0f, 0.2f, 0.8f, 0.2f},
		{ hw, hh, 0.0f, 0.2f, 0.2f, 0.8f},
		{-hw, hh, 0.0f, 0.8f, 0.8f, 0.2f}
	};
	std::vector<GLuint> idx = {0, 1, 2, 2, 3, 0};

	glGenVertexArrays(1, &g_RectMesh.vao);
	glBindVertexArray(g_RectMesh.vao);
	glGenBuffers(1, &g_RectMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_RectMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(V), verts.data(), GL_STATIC_DRAW);
	glGenBuffers(1, &g_RectMesh.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_RectMesh.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0); // pos
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), nullptr);
	glEnableVertexAttribArray(1); // color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), reinterpret_cast<void*>(3 * sizeof(float)));
	g_RectMesh.idxCount = static_cast<int>(idx.size());
	glBindVertexArray(0);
}

static void CreateLineMesh()
{
	// simple line with two vertices, will update dynamically
	struct LV
	{
		float x, y, z;
		float r, g, b;
	};
	std::vector<LV> verts(2);

	glGenVertexArrays(1, &g_LineMesh.vao);
	glBindVertexArray(g_LineMesh.vao);
	glGenBuffers(1, &g_LineMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_LineMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(LV) * 2, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LV), nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LV), reinterpret_cast<void*>(3 * sizeof(float)));
	g_LineMesh.idxCount = 2;
	glBindVertexArray(0);
}

static void CreatePointMesh()
{
	// simple line with two vertices, will update dynamically
	struct PV
	{
		float x, y, z;
		float r, g, b;
	};
	std::vector<PV> verts(1);

	glGenVertexArrays(1, &g_PointMesh.vao);
	glBindVertexArray(g_PointMesh.vao);
	glGenBuffers(1, &g_PointMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_PointMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(PV), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PV), nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PV), reinterpret_cast<void*>(3 * sizeof(float)));
	g_PointMesh.idxCount = 1;
	glBindVertexArray(0);
}

// Draw helpers
static void DrawMesh(const Mesh& m, const glm::mat4& mvp)
{
	g_shaders.activate();
	g_shaders.set_uniform("u_mvp", mvp);
	glBindVertexArray(m.vao);
	glDrawElements(GL_TRIANGLES, m.idxCount, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	g_shaders.deactivate();
}

static void DrawLines(const Mesh& m, const glm::mat4& mvp)
{
	g_shaders.activate();
	g_shaders.set_uniform("u_mvp", mvp);
	glBindVertexArray(m.vao);
	glDrawArrays(GL_LINES, 0, m.idxCount);
	glBindVertexArray(0);
	g_shaders.deactivate();
}

static void DrawPoints(const Mesh& m, const glm::mat4& mvp)
{
	g_shaders.activate();
	g_shaders.set_uniform("u_mvp", mvp);
	glBindVertexArray(m.vao);
	glPointSize(8.0f);
	glDrawArrays(GL_POINTS, 0, m.idxCount);
	glBindVertexArray(0);
	g_shaders.deactivate();
}

// Update dynamic line buffer (controller ray)
static void UpdateLineBuffer(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color)
{
	struct LV
	{
		float x, y, z;
		float r, g, b;
	};
	LV d[2];

	d[0] = {a.x, a.y, a.z, color.r, color.g, color.b};
	d[1] = {b.x, b.y, b.z, color.r, color.g, color.b};
	glBindBuffer(GL_ARRAY_BUFFER, g_LineMesh.vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Update dynamic point buffer (controller plane intersection)
static void UpdatePointBuffer(const glm::vec3& a, const glm::vec3& color)
{
	struct PV
	{
		float x, y, z;
		float r, g, b;
	};
	PV d[1];

	d[0] = {a.x, a.y, a.z, color.r, color.g, color.b};
	glBindBuffer(GL_ARRAY_BUFFER, g_PointMesh.vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static ray_t PointingDirection(const glm::mat4& pose)
{
	ray_t ray;

	// controller ray: origin = device position, direction = forward -Z in device space transformed to world
	ray.origin = glm::vec3(pose * glm::vec4(0, 0, 0, 1));
	ray.direction = glm::normalize(glm::vec3(pose * glm::vec4(0, 0, -1, 0)));

	return ray;
}

static intersection_t ControllerRectangleIntersection(const ray_t& ray, const glm::mat4& model)
{
	intersection_t isec = {false, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)};

	// Transform ray into rectangle local space.
	// Afterwards, collision can be checked by intersection with the x/y plane at z=0.
	glm::mat4 modelInv = glm::inverse(model);
	ray_t local = {
		glm::vec3(modelInv * glm::vec4(ray.origin, 1.0f)),
		glm::normalize(glm::vec3(modelInv * glm::vec4(ray.direction, 0.0f))) // ignore offset/translation with 0.0
	};

	// Ray-plane intersection: plane z=0 in rectangle local space (rect centered at origin, size +/-0.5)
	// rectangle in local coordinates lies on plane z=0

	// ray parallel to z=0 plane.
	if (fabsf(local.direction.z) < std::numeric_limits<float>::epsilon())
	{
		return isec;
	}

	const float t = -local.origin.z / local.direction.z;

	// point of intersection in local coordinates
	// check if it lies within the object boundaries
	const glm::vec2 size(1.0f, 1.0f);
	isec.global = local.origin + t * local.direction;
	isec.local = (glm::vec2(isec.global) + 0.5f * size) / size;

	isec.hit = ((t > 0) &&   // target plane must be in positive direction
		(isec.global.x >= -0.5f * size.x) && (isec.global.x <= 0.5f * size.x) &&
		(isec.global.y >= -0.5f * size.y) && (isec.global.y <= 0.5f * size.y));

	// transform back into global coordinate system
	isec.global = glm::vec3(model * glm::vec4(isec.global, 1.0f));

	return isec;
}

// Main
int main()
{
	// GLFW init
	if (!glfwInit())
	{
		std::cerr << "GLFW init failed" << std::endl;
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	g_Window = glfwCreateWindow(1280, 720, "Cine-VR", nullptr, nullptr);

	if (!g_Window)
	{
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(g_Window);
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		std::cerr << "GLEW init failed" << std::endl;
		return -1;
	}
	glEnable(GL_DEPTH_TEST);

	// Initialize OpenVR
	g_vr.init();
	glm::uvec2 render_size = g_vr.render_target_size();

	for (std::vector<Framebuffer>::iterator iter = g_framebuffer.begin(); iter != g_framebuffer.end(); ++iter)
	{
		iter->init(render_size);
	}

	// create simple shader & geometry
	g_shaders.load_shaders("shaders/scene.vertex.glsl", "shaders/scene.fragment.glsl");
	CreateRectMesh();
	CreateLineMesh();
	CreatePointMesh();

	// main loop
	while (!glfwWindowShouldClose(g_Window) && g_Running)
	{
		glfwPollEvents();

		// handle simple close
		if (glfwGetKey(g_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			break;
		}

		g_vr.read_poses();

		// For each eye: render scene to texture
		for (vr::Hmd_Eye eye : Eyes())
		// for (vr::Hmd_Eye eye = Eyes.begin(); eye != Eyes.end(); eye++)
		{
			Framebuffer& fb = g_framebuffer[eye];
			fb.bind(GL_FRAMEBUFFER);
			glViewport(0, 0, static_cast<GLsizei>(fb.size().x), static_cast<GLsizei>(fb.size().y));
			glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// compute view/proj
			glm::mat4 proj = g_vr.projection(eye);
			glm::mat4 view = g_vr.view(eye);

			// draw rectangle at position in front of HMD at (0,0,-2), rotated slightly
			glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
			model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -1.0f));
			glm::mat4 mvp = proj * view * model;
			DrawMesh(g_RectMesh, mvp);

			// For each controller: render simple ray and do intersection with rectangle
			std::set<vr::TrackedDeviceIndex_t> devices = g_vr.devices();
			for (std::set<vr::TrackedDeviceIndex_t>::const_iterator dev = devices.begin(); dev != devices.end(); ++dev)
			{
				vr::ETrackedDeviceClass devClass = g_vr.device_class(*dev);

				if ((devClass != vr::TrackedDeviceClass_Controller) && (devClass != vr::TrackedDeviceClass_GenericTracker))
				{
					continue;
				}

				const glm::mat4 devPose = g_vr.pose(*dev);
				const ray_t devRay = PointingDirection(devPose);
				const intersection_t isec = ControllerRectangleIntersection(devRay, model);

				// draw ray (in world space)
				UpdateLineBuffer(devRay.origin, devRay.origin + devRay.direction * 5.0f, glm::vec3(0.1f, 0.9f, 0.1f));
				glm::mat4 mvpLine = proj * view * glm::mat4(1.0f);
				DrawLines(g_LineMesh, mvpLine);

				// draw point on panel
				if (isec.hit)
				{
					UpdatePointBuffer(isec.global, glm::vec3(0.2f, 1.0f, 0.2f));
					DrawPoints(g_PointMesh, mvpLine);
				}

				// handle controller input: check trigger press
				vr::VRControllerState_t state = g_vr.controller_state(*dev);

				// typical trigger bit: analog in 'rAxis' or touch; we check trigger button press bit
				bool triggerPressed = (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) != 0;

				if (triggerPressed && isec.hit)
				{
					// hit in rectangle local coords mapped to texture or 0..1 coords
					std::cout << "Controller " << *dev << " clicked rectangle at local coords (u,v)=(" << isec.local.x << "," << isec.local.y << ")" << std::endl;
				}
			}

			fb.unbind(GL_FRAMEBUFFER);
		} // eyes

		// Submit textures to compositor
		for (vr::Hmd_Eye eye : Eyes())
		{
			g_vr.submit(eye, g_framebuffer[eye].texture());
		}

		// blit left eye RT to GLFW window for debug
		int w;
		int h;
		glfwGetFramebufferSize(g_Window, &w, &h);
		glViewport(0, 0, w, h);
		Framebuffer& fb = *g_framebuffer.begin();
		fb.bind(GL_READ_FRAMEBUFFER);
		fb.unbind(GL_DRAW_FRAMEBUFFER);
		glBlitFramebuffer(0, 0, fb.size().x, fb.size().y, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		fb.unbind(GL_READ_FRAMEBUFFER);

		glfwSwapBuffers(g_Window);

		// Let compositor run
		g_vr.handoff();
	}

	// Cleanup
	glfwTerminate();
	return 0;
}
