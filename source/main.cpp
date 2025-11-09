// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <vector>
#include <unistd.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "main.h"
#include "opengl/shader_set.h"
#include "opengl/framebuffer.h"
#include "util/openvr_interface.h"
#include "util/enum_iterator.h"
#include "opengl/shape.h"
#include "opengl/texture.h"
#include "gui/controller.h"
#include "gui/menu.h"
#include "util/file_system.h"

typedef enum
{
	SOURCE_NONE,
	SOURCE_IMAGE,
	SOURCE_VIDEO
}
source_t;

typedef EnumIterator<vr::Hmd_Eye, vr::Eye_Left, vr::Eye_Right> Eyes;

static bool g_Running = true;
static GLFWwindow* g_window = nullptr;
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
static std::string g_current_file_name = "";
static const float g_jump_step = 10.0f;      // seconds
static source_t g_source = SOURCE_NONE;
static Player g_player;
static glm::uvec2 g_window_size(800, 600);

static void framebuffer_size_callback(GLFWwindow* window __attribute__((unused)), int width, int height)
{
	g_window_size = glm::uvec2(width, height);
}

static std::string make_absolute(const std::string& relative)
{
	FileSystem fs;
	const std::string current = fs.current_directory();
	std::vector<std::string> sc = fs.split_path(current);
	std::vector<std::string> sr = fs.split_path(relative);

	sc.insert(sc.end(), sr.begin(), sr.end());
	return fs.join_path(sc.begin(), sc.end());
}

static std::string file_step(const int32_t step)
{
	FileSystem fs;
	std::vector<std::string> path = fs.split_path(g_current_file_name);
	const std::string current_dir = fs.join_path(path.begin(), path.end() - 1);
	const std::string current_file = path[path.size() - 1];
	std::set<std::string> files = fs.file_names(current_dir);
	std::set<std::string>::const_iterator iter = files.find(current_file);

	if (iter == files.end())
	{
		return g_current_file_name;
	}

	if ((step < 0) && (iter == files.begin()))
	{
		return g_current_file_name;
	}

	if (step > 0)
	{
		iter++;
	}
	else if (step < 0)
	{
		iter--;
	}

	if (iter == files.end())
	{
		return g_current_file_name;
	}

	path[path.size() - 1] = *iter;

	return fs.join_path(path.begin(), path.end());
}

void quit(void)
{
	g_player.close();
	g_Running = false;
}

void player_backward(void)
{
	g_player.jump(-g_jump_step);
}

void player_forward(void)
{
	g_player.jump(g_jump_step);
}

void player_pause(void)
{
	g_player.pause();
}

void player_play(void)
{
	g_player.play();
}

void player_previous(void)
{
	const std::string file_name = file_step(-1);

	player_open_file(file_name);
}

void player_next(void)
{
	const std::string file_name = file_step(1);

	player_open_file(file_name);
}

void player_open_file(const std::string& file_name)
{
	FileSystem fs;
	const std::string ext = fs.extension(file_name);

	if (fs.is_image(ext))
	{
		// g_player.stop();
		// g_player.close();
		const glm::uvec2 image_size = g_image.init_image_file(file_name, 0);
		const float aspect = static_cast<float>(image_size.x) / static_cast<float>(image_size.y);
		g_image.unbind();
		g_projection.set_aspect(aspect);
		update_projection();
		g_menu.set_playable(false);
		g_source = SOURCE_IMAGE;
	}
	else if (fs.is_video(ext))
	{
		g_player.open_file(file_name);
		g_menu.set_playable(true);
		g_source = SOURCE_VIDEO;
	}
	g_current_file_name = file_name;
}

void player_show_desktop(void)
{
}

Player& player(void)
{
	return g_player;
}

Projection& projection(void)
{
	return g_projection;
}

void update_projection(void)
{
	std::pair<std::vector<Vertex>, std::vector<GLuint> > proj = g_projection.setup_projection();

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

static void setup_shader(ShaderSet& shader, const vr::Hmd_Eye eye)
{
	glm::vec2 offset;
	glm::vec2 scale;

	if (g_projection.mono() ||
	    (!g_projection.switch_eyes() && (eye == vr::Eye_Left)) ||
	    (g_projection.switch_eyes() && (eye == vr::Eye_Right)))
	{
		switch (g_projection.tiling())
		{
			case Projection::TILE_LEFT_RIGHT:
			case Projection::TILE_CUBE_MAP_STEREO:
				offset = glm::vec2(0.0f, 0.0f);
				scale  = glm::vec2(0.5f, 1.0f);
				break;
			case Projection::TILE_TOP_BOTTOM:
				offset = glm::vec2(0.0f, 0.0f);
				scale  = glm::vec2(1.0f, 0.5f);
				break;
			case Projection::TILE_MONO:
			case Projection::TILE_CUBE_MAP_MONO:
				offset = glm::vec2(0.0f, 0.0f);
				scale  = glm::vec2(1.0f, 1.0f);
				break;
			default:
				throw std::runtime_error("invalid projection");
		}
	}
	else if ((g_projection.switch_eyes() && (eye == vr::Eye_Left)) ||
	         (!g_projection.switch_eyes() && (eye == vr::Eye_Right)))
	{
		switch (g_projection.tiling())
		{
			case Projection::TILE_LEFT_RIGHT:
			case Projection::TILE_CUBE_MAP_STEREO:
				offset = glm::vec2(0.5f, 0.0f);
				scale  = glm::vec2(0.5f, 1.0f);
				// mouse.x += offset.x;
				break;
			case Projection::TILE_TOP_BOTTOM:
				offset = glm::vec2(0.0f, 0.5f);
				scale  = glm::vec2(1.0f, 0.5f);
				// mouse.y += offset.y;
				break;
			case Projection::TILE_MONO:
			case Projection::TILE_CUBE_MAP_MONO:
				offset = glm::vec2(0.0f, 0.0f);
				scale  = glm::vec2(1.0f, 1.0f);
				break;
			default:
				throw std::runtime_error("invalid projection");
		}
	}
	else
	{
		throw std::runtime_error("invalid eye projection");
	}

	// compute view/proj
	const glm::mat4 proj = g_vr.projection(eye);
	const glm::mat4 view = g_vr.view(eye);

	shader.activate();
	shader.set_uniform("projview", proj * view);
	shader.set_uniform("diffuse0", 0);
	shader.set_uniform("texture_offset", offset);
	shader.set_uniform("texture_scale", scale);
}

static void setup_hmd(const glm::mat4& hmdPose)
{
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
}

int main(void)
{
	const std::string initial_file_name = "images/logo-cinevr.png";

	// GLFW init
	if (!glfwInit())
	{
		std::cerr << "GLFW init failed" << std::endl;
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	g_window = glfwCreateWindow(static_cast<int>(g_window_size.x), static_cast<int>(g_window_size.y), "Cine-VR", NULL, NULL);

	if (!g_window)
	{
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(g_window);
	glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);

	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		std::cerr << "GLEW init failed" << std::endl;
		return -1;
	}

	// Initialize OpenVR
	g_vr.init();
	glm::uvec2 render_size = g_vr.render_target_size();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (std::vector<Framebuffer>::iterator iter = g_framebuffer.begin(); iter != g_framebuffer.end(); ++iter)
	{
		iter->init(render_size);
	}

	// create shaders & geometry
	g_shaders.load_shaders("shaders/scene.vertex.glsl", "shaders/scene.fragment.glsl");
	g_shaders.set_uniform("background", false);
	g_shaders.set_uniform("greyscale", false);

	g_menu.init();

	reset_reference();
	g_projection.set_stretch(true);

	player_open_file(make_absolute(initial_file_name));

	// main loop
	while (g_Running)
	{
		// read user inputs
		g_vr.update();
		g_vr.read_poses();

		const OpenVRInterface::input_state_t& input_state = g_vr.read_input();
		float length = glm::length(input_state.pad.position);

		if (!g_menu.active() && (length > 0.5f))
		{
			if ((g_source == SOURCE_VIDEO) && (fabsf(input_state.pad.position.x) > fabsf(input_state.pad.position.y)))
			{
				const int sign = static_cast<int>(input_state.pad.position.x / fabsf(input_state.pad.position.x));
				g_player.jump(static_cast<float>(sign) * g_jump_step);
			}
			else
			{
				const glm::vec2 step = input_state.pad.position * 0.01f / length;
				const float cx = cosf(step.x);
				const float sx = sinf(step.x);
				const float cy = cosf(step.y);
				const float sy = sinf(step.y);
				const glm::quat rotx(cx, 0.0f, sx, 0.0f);
				const glm::quat roty(cy, -sy, 0.0f, 0.0f);

				g_hmd_reference_rot = rotx * g_hmd_reference_rot * roty;
			}
		}

		if ((length < 0.5f) && input_state.pad.button.released)
		{
			reset_reference();
		}

		if (input_state.menu.released)
		{
			std::cout << "action: menu" << std::endl;
		}

		if (input_state.system.released)
		{
			std::cout << "action: system" << std::endl;
		}

		if (input_state.grip.released)
		{
			g_player.pause();
			std::cout << "action: grip" << std::endl;
		}

		if (input_state.trigger.button.released)
		{
			g_vr.haptic(OpenVRInterface::HAPTIC_LEFT);
			g_vr.haptic(OpenVRInterface::HAPTIC_RIGHT);
		}

		if (glm::length(input_state.trigger.value) > 0.0f)
		{
			std::cout << "action: trigger: " << input_state.trigger.value << std::endl;
		}

		g_player.handle_events();

		/* restore transparency */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
			g_menu.checkMenuInteraction(devPose, hmdPose, input_state);
		}

		setup_hmd(hmdPose);

		// For each eye: render scene to texture
		for (vr::Hmd_Eye eye : Eyes())
		{
			Framebuffer& fb = g_framebuffer[eye];
			fb.bind(GL_FRAMEBUFFER);
			glViewport(0, 0, static_cast<GLsizei>(fb.size().x), static_cast<GLsizei>(fb.size().y));
			glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glPointSize(8.0f);

			setup_shader(g_shaders, eye);

			switch (g_source)
			{
				case SOURCE_IMAGE:
					g_image.bind();
					g_canvas.draw();
					g_image.unbind();
					break;
				case SOURCE_VIDEO:
					g_player.bind();
					g_canvas.draw();
					g_player.unbind();
					break;
				default:
					break;
			}

			/* reset to monoscopic mode for menu */
			g_shaders.set_uniform("texture_offset", glm::vec2(0.0f, 0.0f));
			g_shaders.set_uniform("texture_scale",  glm::vec2(1.0f, 1.0f));

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
		glfwGetFramebufferSize(g_window, &w, &h);
		glViewport(0, 0, w, h);
		Framebuffer& fb = *g_framebuffer.begin();
		fb.bind(GL_READ_FRAMEBUFFER);
		fb.unbind(GL_DRAW_FRAMEBUFFER);
		glBlitFramebuffer(0, 0, fb.size().x, fb.size().y, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		fb.unbind(GL_READ_FRAMEBUFFER);

		glfwSwapBuffers(g_window);

		// Let compositor run
		g_vr.handoff();
	}

	// Cleanup
	glfwTerminate();
	return 0;
}
