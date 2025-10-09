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

#include "main.h"
#include "opengl/shader_set.h"
#include "opengl/framebuffer.h"
#include "util/openvr_interface.h"
#include "util/enum_iterator.h"
#include "opengl/shape.h"
#include "gui/button.h"
#include "gui/controller.h"
#include "gui/menu.h"

// #define DEBUG_LINE std::cout << "########## " << __FILE__ << "(" << __LINE__ << "): " << __FUNCTION__ << "()" << std::endl

typedef EnumIterator<vr::Hmd_Eye, vr::Eye_Left, vr::Eye_Right> Eyes;

static bool g_Running = true;
static GLFWwindow* g_Window = nullptr;
static OpenVRInterface g_vr;
static ShaderSet g_shaders;
static std::map<vr::TrackedDeviceIndex_t, Controller> g_controller;
static std::vector<Framebuffer> g_framebuffer(Eyes::size());
static Menu g_menu;
static Projection g_projection;
static Shape g_canvas;
static Texture g_image;
static glm::vec3 g_hmd_reference_pos;
static glm::quat g_hmd_reference_rot;

void quit(void)
{
	g_Running = false;
}

void player_backward(void)
{
}

void player_forward(void)
{
}

void player_pause(void)
{
}

void player_play(void)
{
}

void player_previous(void)
{
}

void player_next(void)
{
}

Projection& projection(void)
{
	return g_projection;
}

void update_projection(void)
{
	std::pair<std::vector<Vertex>, std::vector<GLuint> > proj = g_projection.setup_projection(1.0f);

	g_canvas.init_vertices(proj.first, proj.second);
}

ShaderSet& shader(void)
{
	return g_shaders;
}

static void reset_reference(void)
{
	glm::mat4 hmd_pose = g_vr.pose(vr::k_unTrackedDeviceIndex_Hmd);

	g_hmd_reference_pos = hmd_pose * glm::vec4(0, 0, 0, 1);
	g_hmd_reference_rot = glm::quat_cast(hmd_pose);
}

int main(void)
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
	g_shaders.set_uniform("color_fade", 0.0f);
	g_shaders.set_uniform("greyscale", false);

	g_menu.init();

	reset_reference();
	g_projection.set_stretch(true);
	update_projection();

	g_image.init_file("frame.png", GL_TEXTURE_2D, 0);
	g_image.unbind();

	// main loop
	while (g_Running)
	{
		// read user inputs
		g_vr.update();
		g_vr.read_poses();

		glm::vec3 trackpad = g_vr.getButtonPosition(OpenVRInterface::ACTION_ANALOG);
		float length = glm::length(trackpad);

		if (length > 0.5f)
		{
			const glm::vec2 step = glm::vec2(trackpad) * 0.01f / length;
			const float cx = cosf(step.x);
			const float sx = sinf(step.x);
			const float cy = cosf(step.y);
			const float sy = sinf(step.y);
			const glm::quat rotx(cx, 0.0f, sx, 0.0f);
			const glm::quat roty(cy, -sy, 0.0f, 0.0f);

			g_hmd_reference_rot = rotx * g_hmd_reference_rot * roty;
		}

		if ((length < 0.5f) && g_vr.getButtonAction(OpenVRInterface::ACTION_PADCLICK))
		{
			reset_reference();
		}

		if (g_vr.getButtonAction(OpenVRInterface::ACTION_MENU))
		{
			std::cout << "action: menu" << std::endl;
		}

		if (g_vr.getButtonAction(OpenVRInterface::ACTION_SYSTEM))
		{
			std::cout << "action: system" << std::endl;
		}

		if (g_vr.getButtonAction(OpenVRInterface::ACTION_GRIP))
		{
			std::cout << "action: grip" << std::endl;
		}

		const bool triggerPressed = g_vr.getButtonAction(OpenVRInterface::ACTION_TRIGGER, false);
		const bool triggerReleased = g_vr.getButtonAction(OpenVRInterface::ACTION_TRIGGER, true);

		if (triggerReleased)
		{
			std::cout << "action: trigger" << std::endl;
			g_vr.haptic(OpenVRInterface::ACTION_HAPTIC_LEFT);
			g_vr.haptic(OpenVRInterface::ACTION_HAPTIC_RIGHT);
		}

		glm::vec3 trigger = g_vr.getButtonPosition(OpenVRInterface::ACTION_TRIGGER_VALUE);

		if (glm::length(trigger) > 0.0f)
		{
			std::cout << "action: trigger: " << trigger.x << std::endl;
		}

		const glm::mat4 hmdPose = g_vr.pose(vr::k_unTrackedDeviceIndex_Hmd);
		std::set<vr::TrackedDeviceIndex_t> devices = g_vr.devices();
		for (std::set<vr::TrackedDeviceIndex_t>::const_iterator dev = devices.begin(); dev != devices.end(); ++dev)
		{
			vr::ETrackedDeviceClass devClass = g_vr.device_class(*dev);

			if ((devClass != vr::TrackedDeviceClass_Controller) && (devClass != vr::TrackedDeviceClass_GenericTracker))
			{
				continue;
			}

			std::map<vr::TrackedDeviceIndex_t, Controller>::iterator control = g_controller.find(*dev);

			if (control == g_controller.end())
			{
				std::pair<const vr::TrackedDeviceIndex_t, Controller> p(*dev, Controller());
				control = g_controller.insert(p).first;

				const std::string name = g_vr.name(*dev);
				control->second.init(name);
			}

			const glm::mat4 devPose = g_vr.pose(*dev);
			control->second.set_transform(devPose);

			// check menu interaction
			// TODO: check for trigger of correct controller
			g_menu.checkMenuInteraction(devPose, hmdPose, triggerReleased, triggerPressed);
		}

		glm::mat4 reference;

		if (g_projection.follow_hmd())
		{
			glm::vec4 hmd_position = hmdPose * glm::vec4(0, 0, 0, 1);
			reference = glm::translate(glm::mat4(1.0f), glm::vec3(hmd_position));
		}
		else
		{
			reference = translate(glm::mat4(1.0f), g_hmd_reference_pos);
		}
		reference *= mat4_cast(g_hmd_reference_rot);

		g_canvas.set_transform(reference);

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
			g_shaders.set_uniform("diffuse0", 0);

			g_image.bind();
			g_canvas.draw();
			g_image.unbind();

			g_menu.draw();

			// For each controller: render simple ray and do intersection with rectangle
			for (std::map<vr::TrackedDeviceIndex_t, Controller>::const_iterator iter = g_controller.begin(); iter != g_controller.end(); ++iter)
			{
				iter->second.draw();
			}
			g_shaders.deactivate();

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
