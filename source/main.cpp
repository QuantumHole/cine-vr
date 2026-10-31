// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define GL_GLEXT_PROTOTYPES

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include "main.h"
#include "opengl/shader_set.h"
#include "opengl/framebuffer.h"
#include "util/openvr_interface.h"
#include "util/enum_iterator.h"
#include "opengl/shape.h"
#include "opengl/texture.h"
#include "opengl/render_manager.h"
#include "gui/controller.h"
#include "gui/menu.h"
#include "gui/graphics_svg.h"
#include "util/file_system.h"

typedef enum
{
	SOURCE_NONE,
	SOURCE_IMAGE,
	SOURCE_VIDEO,
	SOURCE_VECTOR
}
source_t;

typedef EnumIterator<vr::Hmd_Eye, vr::Eye_Left, vr::Eye_Right> Eyes;

static bool g_Running = true;
static GLFWwindow* g_window = nullptr;
static OpenVRInterface g_vr;
static size_t g_scene_shader = 0;
static size_t g_image_texture = 0;
static size_t g_video_texture = 0;
static size_t g_scene_shape_set = 0;
static RenderManager::RenderableId g_scene_renderable = 0;
static bool g_scene_ready = false;
static std::map<vr::TrackedDeviceIndex_t, Controller> g_controller;
static std::vector<Framebuffer> g_framebuffer(Eyes::size());
static Menu g_menu;
static Projection g_projection;
static std::unique_ptr<GraphicsSVG> g_vector;
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
		Texture& image = RenderManager::instance().texture(g_image_texture);
		const glm::uvec2 image_size = image.init_image_file(file_name, 0);
		const float aspect = static_cast<float>(image_size.x) / static_cast<float>(image_size.y);
		image.unbind();
		g_projection.set_aspect(aspect);
		g_source = SOURCE_IMAGE;
		update_projection();
		g_menu.set_playable(false);

		if (g_scene_ready)
		{
			RenderManager::instance().set_textures(g_scene_renderable, std::vector<size_t>(1, g_image_texture));
			RenderManager::instance().enable(g_scene_renderable, true);
		}
	}
	else if (fs.is_vector(ext))
	{
		std::unique_ptr<GraphicsSVG> svg;
		try
		{
			svg.reset(new GraphicsSVG(file_name));
		}
		catch (const std::exception& error)
		{
			std::cerr << "Failed to load SVG '" << file_name << "': " << error.what() << std::endl;
			return;
		}

		if (g_source == SOURCE_VIDEO)
		{
			g_player.close();
		}
		g_vector.swap(svg);
		g_menu.set_playable(false);
		g_source = SOURCE_VECTOR;

		if (g_scene_ready)
		{
			RenderManager::instance().enable(g_scene_renderable, false);
		}
	}
	else if (fs.is_video(ext))
	{
		g_player.open_file(file_name, g_window, g_video_texture);
		g_menu.set_playable(true);
		g_source = SOURCE_VIDEO;

		if (g_scene_ready)
		{
			RenderManager::instance().set_textures(g_scene_renderable, std::vector<size_t>(1, g_video_texture));
			RenderManager::instance().enable(g_scene_renderable, true);
		}
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

ShaderSet& shader(void)
{
	return RenderManager::instance().shader(g_scene_shader);
}

void update_projection(void)
{
	std::pair<std::vector<Vertex>, std::vector<GLuint> > proj = g_projection.setup_projection();
	RenderManager& renderer = RenderManager::instance();
	const size_t new_shape_set = renderer.create_shape_set();

	renderer.add_shape(new_shape_set, proj.first, proj.second);
	std::vector<size_t> textures;

	if (g_source == SOURCE_IMAGE)
	{
		textures.push_back(g_image_texture);
	}
	else if (g_source == SOURCE_VIDEO)
	{
		textures.push_back(g_video_texture);
	}
	const RenderManager::RenderableId new_renderable =
		renderer.create_shape_set_renderable(new_shape_set, textures, g_scene_shader);

	if (g_scene_ready)
	{
		renderer.delete_renderable(g_scene_renderable);
	}
	g_scene_shape_set = new_shape_set;
	g_scene_renderable = new_renderable;
	g_scene_ready = true;
}

static void reset_reference(void)
{
	glm::mat4 hmd_pose = g_vr.pose(vr::k_unTrackedDeviceIndex_Hmd);

	g_hmd_reference_pos = hmd_pose * glm::vec4(0, 0, 0, 1);
	g_hmd_reference_rot = glm::quat_cast(hmd_pose);
}

static void setup_shader(const ShaderSet& shader, const vr::Hmd_Eye eye)
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

	if (g_scene_ready)
	{
		RenderManager::instance().set_transform(g_scene_shape_set, 0, reference);
	}

	if (g_vector)
	{
		const glm::mat4 svg_transform = reference *
		                                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f)) *
		                                glm::scale(glm::mat4(1.0f), glm::vec3(5.0f));
		g_vector->set_transform(svg_transform);
	}
}

int main(void)
{
	const std::string initial_file_name = "images/logo-cinevr.svg";

	const char* platform_override = std::getenv("CINE_VR_PLATFORM");

	if (platform_override != nullptr)
	{
		if (std::strcmp(platform_override, "wayland") == 0)
		{
			glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
		}
		else if (std::strcmp(platform_override, "x11") == 0)
		{
			glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
		}
		else if (std::strcmp(platform_override, "auto") != 0)
		{
			std::cerr << "Invalid CINE_VR_PLATFORM value: " << platform_override
			          << " (expected wayland, x11, or auto); using GLFW auto-selection" << std::endl;
			glfwInitHint(GLFW_PLATFORM, GLFW_ANY_PLATFORM);
		}
	}
	else if (std::getenv("WAYLAND_DISPLAY") != nullptr)
	{
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
	}

	// GLFW init
	if (!glfwInit())
	{
		std::cerr << "GLFW initialization failed; check the selected platform and "
		          << "ensure GLFW was built with Wayland support when using sway" << std::endl;
		return -1;
	}
	const int glfw_platform = glfwGetPlatform();
	std::cout << "GLFW platform: "
	          << (glfw_platform == GLFW_PLATFORM_WAYLAND ? "Wayland" :
	    glfw_platform == GLFW_PLATFORM_X11 ? "X11" : "other")
	          << std::endl;
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

	// Initialize OpenVR
	try
	{
		g_vr.init();
	}
	catch (const std::exception& error)
	{
		std::cerr << "OpenVR initialization failed: " << error.what() << std::endl;
		glfwDestroyWindow(g_window);
		glfwTerminate();
		return -1;
	}
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
	RenderManager& renderer = RenderManager::instance();
	g_scene_shader = renderer.add_shaders("shaders/scene.vertex.glsl", "shaders/scene.fragment.glsl");
	g_image_texture = renderer.create_texture();
	g_video_texture = renderer.create_texture();
	ShaderSet& scene_shader = renderer.shader(g_scene_shader);
	scene_shader.set_uniform("background", false);
	scene_shader.set_uniform("greyscale", false);

	g_menu.init();

	reset_reference();
	g_projection.set_stretch(true);

	player_open_file(make_absolute(initial_file_name));

	// main loop
	while (g_Running)
	{
		glfwPollEvents();

		if (glfwWindowShouldClose(g_window))
		{
			g_Running = false;
			break;
		}

		// read user inputs
		g_vr.update();
		g_vr.read_poses();

		const OpenVRInterface::input_state_t& input_state = g_vr.read_input();
		float length = glm::length(input_state.pad.position);

		if (!g_menu.active() && (length > 0.5f))
		{
			if ((g_source == SOURCE_VIDEO) &&
			    (input_state.pad.button.released) &&
			    (fabsf(input_state.pad.position.x) > fabsf(input_state.pad.position.y)))
			{
				const int sign = static_cast<int>(input_state.pad.position.x / fabsf(input_state.pad.position.x));
				g_player.jump(static_cast<float>(sign) * g_jump_step);
			}
			else if ((g_source == SOURCE_IMAGE) ||
			         (fabsf(input_state.pad.position.x) < fabsf(input_state.pad.position.y)))
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
			if (g_player.is_playing())
			{
				g_player.pause();
			}
			else
			{
				g_player.play();
			}
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

			setup_shader(scene_shader, eye);
			switch (g_source)
			{
				case SOURCE_IMAGE:
					renderer.draw();
					break;
				case SOURCE_VIDEO:
					renderer.draw();
					break;
				case SOURCE_VECTOR:

					if (g_vector)
					{
						g_vector->draw();
					}
					break;
				default:
					break;
			}

			if (g_menu.active())
			{
				/* reset to monoscopic mode for menu */
				scene_shader.set_uniform("texture_offset", glm::vec2(0.0f, 0.0f));
				scene_shader.set_uniform("texture_scale",  glm::vec2(1.0f, 1.0f));

				g_menu.draw();

				// For each controller: render simple ray and do intersection with rectangle
				// OpenVR object definitions are compatible with the Vertex structure
				// Therefore, it can be rendered with the same shader
				for (std::map<vr::TrackedDeviceIndex_t, Controller>::const_iterator iter = g_controller.begin(); iter != g_controller.end(); ++iter)
				{
					iter->second.draw();
				}
			}
			scene_shader.deactivate();

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
		glBlitFramebuffer(0, 0, static_cast<GLint>(fb.size().x), static_cast<GLint>(fb.size().y), 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		fb.unbind(GL_READ_FRAMEBUFFER);

		glfwSwapBuffers(g_window);

		// Let compositor run
		g_vr.handoff();
	}

	// Cleanup
	g_menu.clear();
	g_vector.reset();
	renderer.clear();
	glfwTerminate();
	return 0;
}
