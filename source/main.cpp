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
#include "controller.h"

typedef EnumIterator<vr::Hmd_Eye, vr::Eye_Left, vr::Eye_Right> Eyes;

static bool g_Running = true;
static bool g_show_controls = false;
static GLFWwindow* g_Window = nullptr;
static OpenVRInterface g_vr;
static ShaderSet g_shaders;
static std::vector<Button> g_button;
static std::map<vr::TrackedDeviceIndex_t, Controller> g_controller;
static std::vector<Framebuffer> g_framebuffer(Eyes::size());
static Shape g_points;

static void CreateButtonRow(const float size, const std::vector<Button::button_action_t>& actions)
{
	const size_t num = actions.size();
	const float range = 0.5f * glm::pi<float>();
	const float step = range / static_cast<float>(num);

	// g_vr.read_poses();
	const glm::mat4 hmdPose = g_vr.pose(vr::k_unTrackedDeviceIndex_Hmd);

	g_button.resize(num);
	float angle = 0.5f * range - 0.5f * step;
	size_t i = 0;
	for (std::vector<Button>::iterator iter = g_button.begin(); iter != g_button.end(); ++iter)
	{
		glm::mat4 pose = glm::mat4(1.0f);
		pose = glm::rotate(pose, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		pose = glm::translate(pose, glm::vec3(0.0f, 0.0f, -5.0f));
		pose = glm::rotate(pose, -0.2f * glm::pi<float>(), glm::vec3(0.1f, 0.0f, 0.0f));   // tilt panels
		pose = hmdPose * pose;

		iter->init(size, actions[i]);
		iter->set_transform(pose);

		angle -= step;
		i++;
	}
}

static void CreatePointMesh(void)
{
	// simple line with two vertices, will update dynamically
	std::vector<Vertex> vertices = {
		Vertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.2f, 1.0f, 0.2f), glm::vec2(0.0f, 0.0f)),
	};
	const std::vector<GLuint> indices = {
		0,
	};

	g_points.init_vertices(vertices, indices, GL_POINTS);
}

static void handleButtonAction(const Button::button_action_t action)
{
	switch (action)
	{
		case Button::BUTTON_BACKWARD:
			break;
		case Button::BUTTON_CUBE_MONO:
			break;
		case Button::BUTTON_CUBE_STEREO:
			break;
		case Button::BUTTON_CYLINDER:
			break;
		case Button::BUTTON_DELETE:
			break;
		case Button::BUTTON_FISHEYE:
			break;
		case Button::BUTTON_FLAT:
			break;
		case Button::BUTTON_FORWARD:
			break;
		case Button::BUTTON_LEFT_RIGHT:
			break;
		case Button::BUTTON_MONO:
			break;
		case Button::BUTTON_NEXT:
			break;
		case Button::BUTTON_OPEN:
			break;
		case Button::BUTTON_PAUSE:
			break;
		case Button::BUTTON_PLAY:
			break;
		case Button::BUTTON_POWER:
			g_Running = false;
			break;
		case Button::BUTTON_PREVIOUS:
			break;
		case Button::BUTTON_SPHERE:
			break;
		case Button::BUTTON_TOP_BOTTOM:
			break;
		default:
			break;
	}
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

	// draw rectangle at position in front of HMD at (0,0,-2), rotated slightly
	// const glm::vec2 panel_size(1.0f, 1.0f);
	// CreateRectMesh(panel_size, 5);
	CreatePointMesh();

	// main loop
	while (g_Running)
	{
		// read user inputs
		g_vr.update();
		g_vr.read_poses();

		bool triggerPressed = false;

		if (g_vr.getButtonAction(OpenVRInterface::ACTION_PADCLICK))
		{
			std::cout << "action: pad click" << std::endl;
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

		if (g_vr.getButtonAction(OpenVRInterface::ACTION_TRIGGER))
		{
			std::cout << "action: trigger" << std::endl;
			triggerPressed = true;
			g_vr.haptic(OpenVRInterface::ACTION_HAPTIC_LEFT);
			g_vr.haptic(OpenVRInterface::ACTION_HAPTIC_RIGHT);
		}

		glm::vec3 trackpad = g_vr.getButtonPosition(OpenVRInterface::ACTION_ANALOG);

		if (glm::length(trackpad) > 0.0f)
		{
			std::cout << "action: analog: " << trackpad.x << " / " << trackpad.y << std::endl;
		}
		glm::vec3 trigger = g_vr.getButtonPosition(OpenVRInterface::ACTION_TRIGGER_VALUE);

		if (glm::length(trigger) > 0.0f)
		{
			std::cout << "action: trigger: " << trigger.x << std::endl;
		}

		bool buttonHit = false;
		std::vector<glm::vec3> intersections;
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

			for (std::vector<Button>::iterator iter = g_button.begin(); iter != g_button.end(); ++iter)
			{
				const Button::intersection_t isec = iter->intersection(devPose);

				// draw point on panel
				if (isec.hit)
				{
					buttonHit |= isec.hit;
					// std::cout << "panel hit at (" << isec.global.x << ", " << isec.global.y << ", " << isec.global.z << ")" << std::endl;
					intersections.push_back(isec.global);
				}

				if (triggerPressed && isec.hit)
				{
					// hit in rectangle local coords mapped to texture or 0..1 coords
					std::cout << "Controller " << *dev << " clicked button " << isec.button_id << " at local coords (u,v)=(" << isec.local.x << "," << isec.local.y << ")" << std::endl;
					handleButtonAction(isec.action_id);
				}
			}
		}

		// perform user actions
		if (triggerPressed)
		{
			if (!g_show_controls)
			{
				g_show_controls = true;
				CreateButtonRow(1.0f, {
					Button::BUTTON_PREVIOUS,
					Button::BUTTON_BACKWARD,
					Button::BUTTON_PLAY,
					Button::BUTTON_FORWARD,
					Button::BUTTON_NEXT,
					Button::BUTTON_POWER
				});
			}
			else if (!buttonHit)
			{
				g_show_controls = false;
				g_button.clear();
			}
		}

		// update scene objects
		// define all intersection points
		g_points.set_instances(intersections.size());
		for (size_t i = 0; i < intersections.size(); i++)
		{
			const glm::mat4 popo = glm::translate(glm::mat4(1.0f), intersections[i]);
			g_points.set_transform(popo, i);
		}

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
			for (std::vector<Button>::iterator iter = g_button.begin(); iter != g_button.end(); ++iter)
			{
				iter->draw();
			}

			// For each controller: render simple ray and do intersection with rectangle
			for (std::map<vr::TrackedDeviceIndex_t, Controller>::const_iterator iter = g_controller.begin(); iter != g_controller.end(); ++iter)
			{
				iter->second.draw();
			}
			g_points.draw();
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
