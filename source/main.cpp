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
#include "opengl/shape.h"
#include "button.h"

typedef EnumIterator<vr::Hmd_Eye, vr::Eye_Left, vr::Eye_Right> Eyes;

static bool g_Running = true;
static GLFWwindow* g_Window = nullptr;
static OpenVRInterface g_vr;
static ShaderSet g_shaders;
static std::vector<Button> g_button;
static Shape g_shape_line;
static Shape g_shape_point;
static std::vector<Framebuffer> g_framebuffer(Eyes::size());

static void CreateRectMesh(const glm::vec2& size, const size_t num)
{
	const float range = 0.5f * glm::pi<float>();
	const float step = range / static_cast<float>(num);

	g_vr.read_poses();
	const glm::mat4 hmdPose = g_vr.pose(vr::k_unTrackedDeviceIndex_Hmd);

	g_button.resize(num);
	float angle = 0.5f * step - 0.5f * range;
	for (std::vector<Button>::iterator iter = g_button.begin(); iter != g_button.end(); ++iter)
	{
		glm::mat4 pose = glm::mat4(1.0f);
		pose = glm::rotate(pose, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
		pose = glm::rotate(pose, -0.2f * glm::pi<float>(), glm::vec3(0.1f, 0.0f, 0.0f));   // tilt panels
		pose = hmdPose * pose;

		iter->init(size);
		iter->set_transform(pose);

		angle += step;
	}
}

static void CreateLineMesh()
{
	// simple line with two vertices, will update dynamically
	std::vector<Vertex> vertices = {
		Vertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.1f, 0.9f, 0.1f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.1f, 0.9f, 0.1f), glm::vec2(0.0f, 0.0f)),
	};

	const std::vector<GLuint> indices = {
		0, 1
	};

	g_shape_line.init_vertices(vertices, indices, GL_LINES);
}

static void CreatePointMesh()
{
	// simple line with two vertices, will update dynamically
	std::vector<Vertex> vertices = {
		Vertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.2f, 1.0f, 0.2f), glm::vec2(0.0f, 0.0f)),
	};
	const std::vector<GLuint> indices = {
		0,
	};

	g_shape_point.init_vertices(vertices, indices, GL_POINTS);
}

// Draw helpers
static void DrawMesh(Shape& shape, const glm::mat4& projview)
{
	g_shaders.activate();
	g_shaders.set_uniform("projview", projview);

	// g_shaders.set_uniform("texture_offset", glm::vec2(0.0f, 0.0f));
	// g_shaders.set_uniform("texture_scale", glm::vec2(1.0f, 1.0f));
	// g_shaders.set_uniform("cursor_location", glm::vec2(0.0f, 0.0f));
	// g_shaders.set_uniform("arrow_size", glm::vec2(1.0f, 1.0f));
	// g_shaders.set_uniform("diffuse0", 0);
	// g_shaders.set_uniform("arrow_texture", 1);

	shape.draw();
	g_shaders.deactivate();
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

	// draw rectangle at position in front of HMD at (0,0,-2), rotated slightly
	const glm::vec2 panel_size(1.0f, 1.0f);
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
	model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, glm::vec3(-3.0f, 1.5f, 0.0f));

	CreateRectMesh(panel_size, 5);
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
		{
			Framebuffer& fb = g_framebuffer[eye];
			fb.bind(GL_FRAMEBUFFER);
			glViewport(0, 0, static_cast<GLsizei>(fb.size().x), static_cast<GLsizei>(fb.size().y));
			glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glPointSize(8.0f);

			// compute view/proj
			glm::mat4 proj = g_vr.projection(eye);
			glm::mat4 view = g_vr.view(eye);

			g_shaders.activate();
			g_shaders.set_uniform("projview", proj * view);
			for (std::vector<Button>::iterator iter = g_button.begin(); iter != g_button.end(); ++iter)
			{
				iter->draw();
			}
			g_shaders.deactivate();

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
				g_shape_line.set_transform(devPose);
				DrawMesh(g_shape_line, proj * view);

				// handle controller input: check trigger press
				vr::VRControllerState_t state = g_vr.controller_state(*dev);

				// typical trigger bit: analog in 'rAxis' or touch; we check trigger button press bit
				bool triggerPressed = (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) != 0;

				for (std::vector<Button>::iterator iter = g_button.begin(); iter != g_button.end(); ++iter)
				{
					const Button::intersection_t isec = iter->intersection(devPose);

					// draw point on panel
					if (isec.hit)
					{
						const glm::mat4 popo = glm::translate(glm::mat4(1.0f), isec.global);
						g_shape_point.set_transform(popo);
						DrawMesh(g_shape_point, proj * view);
					}

					if (triggerPressed && isec.hit)
					{
						// hit in rectangle local coords mapped to texture or 0..1 coords
						std::cout << "Controller " << *dev << " clicked button at local coords (u,v)=(" << isec.local.x << "," << isec.local.y << ")" << std::endl;
					}
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
