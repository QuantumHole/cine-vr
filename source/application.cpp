// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application.h"
#include "util/file_system.h"
#include "util/resource_manager.h"

#include <iostream>

typedef EnumIterator<vr::Hmd_Eye, vr::Eye_Left, vr::Eye_Right> Eyes;

Application* Application::m_instance = nullptr;
glm::uvec2 Application::m_window_size(800, 600);

Application::Application(void) :
	m_app_name("cine-vr"),
	m_database(m_app_name),
	m_Running(true),
	m_window(nullptr),
	m_vr(),
	m_scene_shader(0),
	m_image_texture(0),
	m_video_texture(0),
	m_scene_shape_set(0),
	m_scene_renderable(0),
	m_scene_ready(false),
	m_controller(),
	m_framebuffer(Eyes::size()),
	m_menu(),
	m_projection(),
	m_vector(nullptr),
	m_hmd_reference_pos(),
	m_hmd_reference_rot(),
	m_current_file_name(""),
	m_jump_step(10.0f),      // seconds
	m_source(SOURCE_NONE),
	m_player()
{
}

Application& Application::instance(void)
{
	if (!m_instance)
	{
		m_instance = new Application();
	}
	return *m_instance;
}

Application::~Application(void)
{
	if (m_instance == this)
	{
		m_instance = nullptr;
	}
}

const std::string& Application::app_name(void) const
{
	return m_app_name;
}

const SQLiteDB& Application::database(void) const
{
	return m_database;
}

void Application::framebuffer_size_callback(GLFWwindow* window __attribute__((unused)), int width, int height)
{
	m_window_size = glm::uvec2(width, height);
}

void Application::init_camera_pose(void)
{
	float cursor_size;   // not yet implemented.

	m_database.read_camera(m_hmd_reference_pos, m_hmd_reference_rot, cursor_size);
}

void Application::reset_reference(void)
{
	glm::mat4 hmd_pose = m_vr.pose(vr::k_unTrackedDeviceIndex_Hmd);

	m_hmd_reference_pos = hmd_pose * glm::vec4(0, 0, 0, 1);
	m_hmd_reference_rot = glm::quat_cast(hmd_pose);

	const float cursor_size = 0.0f;   // not yet implemented.
	m_database.store_camera(m_hmd_reference_pos, m_hmd_reference_rot, cursor_size);
}

std::string Application::make_absolute(const std::string& relative)
{
	FileSystem fs;
	const std::string current = fs.current_directory();
	std::vector<std::string> sc = fs.split_path(current);
	std::vector<std::string> sr = fs.split_path(relative);

	sc.insert(sc.end(), sr.begin(), sr.end());
	return fs.join_path(sc.begin(), sc.end());
}

std::string Application::file_step(const int32_t step)
{
	FileSystem fs;
	std::vector<std::string> path = fs.split_path(m_current_file_name);
	const std::string current_dir = fs.join_path(path.begin(), path.end() - 1);
	const std::string current_file = path[path.size() - 1];
	std::set<std::string> files = fs.file_names(current_dir);
	std::set<std::string>::const_iterator iter = files.find(current_file);

	if (iter == files.end())
	{
		return m_current_file_name;
	}

	if ((step < 0) && (iter == files.begin()))
	{
		return m_current_file_name;
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
		return m_current_file_name;
	}

	path[path.size() - 1] = *iter;

	return fs.join_path(path.begin(), path.end());
}

void Application::setup_shader(const ShaderSet& shader, const vr::Hmd_Eye eye)
{
	glm::vec2 offset;
	glm::vec2 scale;

	if (m_projection.mono() ||
	    (!m_projection.switch_eyes() && (eye == vr::Eye_Left)) ||
	    (m_projection.switch_eyes() && (eye == vr::Eye_Right)))
	{
		switch (m_projection.tiling())
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
	else if ((m_projection.switch_eyes() && (eye == vr::Eye_Left)) ||
	         (!m_projection.switch_eyes() && (eye == vr::Eye_Right)))
	{
		switch (m_projection.tiling())
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
	const glm::mat4 proj = m_vr.projection(eye);
	const glm::mat4 view = m_vr.view(eye);

	shader.activate();
	shader.set_uniform("projview", proj * view);
	shader.set_uniform("diffuse0", 0);
	shader.set_uniform("texture_offset", offset);
	shader.set_uniform("texture_scale", scale);
}

void Application::setup_hmd(const glm::mat4& hmdPose)
{
	glm::mat4 reference;

	if (m_projection.follow_hmd())
	{
		glm::vec4 hmd_position = hmdPose * glm::vec4(0, 0, 0, 1);
		reference = glm::translate(glm::mat4(1.0f), glm::vec3(hmd_position));
	}
	else
	{
		reference = translate(glm::mat4(1.0f), m_hmd_reference_pos);
	}
	reference *= mat4_cast(m_hmd_reference_rot);

	if (m_scene_ready)
	{
		RenderManager::instance().set_transform(m_scene_shape_set, 0, reference);
	}

	if (m_vector)
	{
		const glm::mat4 svg_transform = reference *
		                                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f)) *
		                                glm::scale(glm::mat4(1.0f), glm::vec3(5.0f));
		m_vector->set_transform(svg_transform);
	}
}

void Application::quit(void)
{
	m_player.close();
	m_Running = false;
}

void Application::player_backward(void)
{
	m_player.jump(-m_jump_step);
}

void Application::player_forward(void)
{
	m_player.jump(m_jump_step);
}

void Application::player_pause(void)
{
	m_player.pause();
}

void Application::player_play(void)
{
	m_player.play();
}

void Application::player_previous(void)
{
	const std::string file_name = file_step(-1);

	player_open_file(file_name);
}

void Application::player_next(void)
{
	const std::string file_name = file_step(1);

	player_open_file(file_name);
}

void Application::player_open_file(const std::string& file_name)
{
	FileSystem fs;
	const std::string ext = fs.extension(file_name);

	if (fs.is_image(ext))
	{
		Texture& image = RenderManager::instance().texture(m_image_texture);
		const glm::uvec2 image_size = image.init_image_file(file_name, 0);
		const float aspect = static_cast<float>(image_size.x) / static_cast<float>(image_size.y);
		image.unbind();
		m_projection.set_aspect(aspect);
		m_source = SOURCE_IMAGE;
		update_projection();
		m_menu.set_playable(false);

		if (m_scene_ready)
		{
			RenderManager::instance().set_textures(m_scene_renderable, std::vector<size_t>(1, m_image_texture));
			RenderManager::instance().enable(m_scene_renderable, true);
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

		if (m_source == SOURCE_VIDEO)
		{
			m_player.close();
		}
		m_vector.swap(svg);
		m_menu.set_playable(false);
		m_source = SOURCE_VECTOR;

		if (m_scene_ready)
		{
			RenderManager::instance().enable(m_scene_renderable, false);
		}
	}
	else if (fs.is_video(ext))
	{
		m_player.open_file(file_name, m_window, m_video_texture);
		m_menu.set_playable(true);
		m_source = SOURCE_VIDEO;

		/** load video projection settings. */
		float duration = 0.0f;
		const size_t video_id = m_database.read_video(file_name, duration);

		if ((video_id > 0) && (duration >= 0.0f))
		{
			size_t projection = 0;
			size_t tiling = 0;
			bool stretch = false;
			float angle = 0.0f;
			float zoom = 0.0f;
			bool switch_eyes = false;
			bool mono = false;
			m_database.read_settings(video_id, projection, tiling, stretch, angle, zoom, switch_eyes, mono);

			m_projection.set_projection(static_cast<Projection::video_projection_t>(projection));
			m_projection.set_tiling(static_cast<Projection::video_tiling_t>(tiling));
			m_projection.set_angle(angle); /* angle given in radians */
			m_projection.set_zoom(zoom);
			m_projection.set_stretch(stretch);
			m_projection.set_switch_eyes(switch_eyes);
			m_projection.set_mono(mono);

			update_projection();
		}

		if (m_scene_ready)
		{
			RenderManager::instance().set_textures(m_scene_renderable, std::vector<size_t>(1, m_video_texture));
			RenderManager::instance().enable(m_scene_renderable, true);
		}
	}
	m_current_file_name = file_name;
}

void Application::player_show_desktop(void)
{
}

Player& Application::player(void)
{
	return m_player;
}

const std::string& Application::current_file_name(void) const
{
	return m_current_file_name;
}

Projection& Application::projection(void)
{
	return m_projection;
}

ShaderSet& Application::shader(void)
{
	return RenderManager::instance().shader(m_scene_shader);
}

void Application::update_projection(void)
{
	std::pair<std::vector<Vertex>, std::vector<GLuint> > proj = m_projection.setup_projection();
	RenderManager& renderer = RenderManager::instance();
	const size_t new_shape_set = renderer.create_shape_set();

	renderer.add_shape(new_shape_set, proj.first, proj.second);
	std::vector<size_t> textures;

	if (m_source == SOURCE_IMAGE)
	{
		textures.push_back(m_image_texture);
	}
	else if (m_source == SOURCE_VIDEO)
	{
		textures.push_back(m_video_texture);

		/** load video projection settings. */
		float duration = 0.0f;
		const size_t video_id = m_database.read_video(m_current_file_name, duration);

		if ((video_id > 0) && (duration >= 0.0f))
		{
			std::cout << "store settings for video ID " << video_id << " - " << m_current_file_name << std::endl;
			const size_t projection = m_projection.projection();
			const size_t tiling     = m_projection.tiling();
			const float angle       = m_projection.angle();
			const float zoom        = m_projection.zoom();
			const bool stretch      = m_projection.stretch();
			const bool switch_eyes  = m_projection.switch_eyes();
			const bool mono         = m_projection.mono();
			m_database.store_settings(video_id, projection, tiling, stretch, angle, zoom, switch_eyes, mono);
		}
	}
	const size_t new_renderable =
		renderer.create_shape_set_renderable(new_shape_set, textures, m_scene_shader);

	if (m_scene_ready)
	{
		renderer.delete_renderable(m_scene_renderable);
	}
	m_scene_shape_set = new_shape_set;
	m_scene_renderable = new_renderable;
	m_scene_ready = true;
}

int Application::run(void)
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
	m_window = glfwCreateWindow(static_cast<int>(m_window_size.x), static_cast<int>(m_window_size.y), "Cine-VR", NULL, NULL);

	if (!m_window)
	{
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(m_window);
	glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

	// Initialize OpenVR
	try
	{
		m_vr.init();
	}
	catch (const std::exception& error)
	{
		std::cerr << "OpenVR initialization failed: " << error.what() << std::endl;
		m_vr.shutdown();
		glfwDestroyWindow(m_window);
		m_window = nullptr;
		glfwTerminate();
		return -1;
	}
	glm::uvec2 render_size = m_vr.render_target_size();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (std::vector<Framebuffer>::iterator iter = m_framebuffer.begin(); iter != m_framebuffer.end(); ++iter)
	{
		iter->init(render_size);
	}

	// load font for Renderer initially
	FontRenderer::instance().load_font("OpenSans");

	// create shaders & geometry
	RenderManager& renderer = RenderManager::instance();
	m_scene_shader = renderer.add_shaders("shaders/scene.vertex.glsl", "shaders/scene.fragment.glsl");
	m_image_texture = renderer.create_texture();
	m_video_texture = renderer.create_texture();
	ShaderSet& scene_shader = renderer.shader(m_scene_shader);
	scene_shader.set_uniform("background", false);
	scene_shader.set_uniform("greyscale", false);

	m_menu.init();

	init_camera_pose();
	m_projection.set_stretch(true);

	player_open_file(make_absolute(initial_file_name));

	// main loop
	while (m_Running)
	{
		glfwPollEvents();

		if (glfwWindowShouldClose(m_window))
		{
			m_Running = false;
			break;
		}

		// read user inputs
		m_vr.update();
		m_vr.read_poses();

		const OpenVRInterface::input_state_t& input_state = m_vr.read_input();
		float length = glm::length(input_state.pad.position);

		if (!m_menu.active() && (length > 0.5f))
		{
			if ((m_source == SOURCE_VIDEO) &&
			    (input_state.pad.button.released) &&
			    (fabsf(input_state.pad.position.x) > fabsf(input_state.pad.position.y)))
			{
				const int sign = static_cast<int>(input_state.pad.position.x / fabsf(input_state.pad.position.x));
				m_player.jump(static_cast<float>(sign) * m_jump_step);
			}
			else if ((m_source == SOURCE_IMAGE) ||
			         (fabsf(input_state.pad.position.x) < fabsf(input_state.pad.position.y)))
			{
				const glm::vec2 step = input_state.pad.position * 0.01f / length;
				const float cx = cosf(step.x);
				const float sx = sinf(step.x);
				const float cy = cosf(step.y);
				const float sy = sinf(step.y);
				const glm::quat rotx(cx, 0.0f, sx, 0.0f);
				const glm::quat roty(cy, -sy, 0.0f, 0.0f);

				m_hmd_reference_rot = rotx * m_hmd_reference_rot * roty;
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
			if (m_player.is_playing())
			{
				m_player.pause();
			}
			else
			{
				m_player.play();
			}
		}

		if (input_state.trigger.button.released)
		{
			m_vr.haptic(OpenVRInterface::HAPTIC_LEFT);
			m_vr.haptic(OpenVRInterface::HAPTIC_RIGHT);
		}

		if (glm::length(input_state.trigger.value) > 0.0f)
		{
			std::cout << "action: trigger: " << input_state.trigger.value << std::endl;
		}

		m_player.begin_frame();
		m_player.handle_events();

		/* restore transparency */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		const glm::mat4 hmdPose = m_vr.pose(vr::k_unTrackedDeviceIndex_Hmd);
		std::set<vr::TrackedDeviceIndex_t> devices = m_vr.devices();
		for (std::set<vr::TrackedDeviceIndex_t>::const_iterator dev = devices.begin(); dev != devices.end(); ++dev)
		{
			vr::ETrackedDeviceClass devClass = m_vr.device_class(*dev);

			if ((devClass != vr::TrackedDeviceClass_Controller) && (devClass != vr::TrackedDeviceClass_GenericTracker))
			{
				continue;
			}

			std::map<vr::TrackedDeviceIndex_t, Controller>::iterator control = m_controller.find(*dev);

			if (control == m_controller.end())
			{
				std::pair<const vr::TrackedDeviceIndex_t, Controller> p(*dev, Controller());
				control = m_controller.insert(p).first;

				const std::string name = m_vr.name(*dev);
				control->second.init(name);
			}

			const glm::mat4 devPose = m_vr.pose(*dev);
			control->second.set_transform(devPose);

			// check menu interaction
			// TODO: check for trigger of correct controller
			m_menu.checkMenuInteraction(devPose, hmdPose, input_state);
		}

		setup_hmd(hmdPose);

		// For each eye: render scene to texture
		for (vr::Hmd_Eye eye : Eyes())
		{
			Framebuffer& fb = m_framebuffer[eye];
			fb.bind(GL_FRAMEBUFFER);
			glViewport(0, 0, static_cast<GLsizei>(fb.size().x), static_cast<GLsizei>(fb.size().y));
			glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glPointSize(8.0f);

			setup_shader(scene_shader, eye);
			switch (m_source)
			{
				case SOURCE_IMAGE:
					renderer.draw();
					break;
				case SOURCE_VIDEO:
					renderer.draw();
					break;
				case SOURCE_VECTOR:

					if (m_vector)
					{
						m_vector->draw();
					}
					break;
				default:
					break;
			}

			if (m_menu.active())
			{
				/* reset to monoscopic mode for menu */
				scene_shader.set_uniform("texture_offset", glm::vec2(0.0f, 0.0f));
				scene_shader.set_uniform("texture_scale",  glm::vec2(1.0f, 1.0f));

				m_menu.draw();

				// For each controller: render simple ray and do intersection with rectangle
				// OpenVR object definitions are compatible with the Vertex structure
				// Therefore, it can be rendered with the same shader
				for (std::map<vr::TrackedDeviceIndex_t, Controller>::const_iterator iter = m_controller.begin(); iter != m_controller.end(); ++iter)
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
			m_vr.submit(eye, m_framebuffer[eye].texture());
		}

		// blit left eye RT to GLFW window for debug
		int w;
		int h;
		glfwGetFramebufferSize(m_window, &w, &h);
		glViewport(0, 0, w, h);
		Framebuffer& fb = *m_framebuffer.begin();
		fb.bind(GL_READ_FRAMEBUFFER);
		fb.unbind(GL_DRAW_FRAMEBUFFER);
		glBlitFramebuffer(0, 0, static_cast<GLint>(fb.size().x), static_cast<GLint>(fb.size().y), 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		fb.unbind(GL_READ_FRAMEBUFFER);

		glfwSwapBuffers(m_window);

		// Let compositor run
		m_vr.handoff();
		m_player.end_frame();
	}

	// Cleanup
	m_player.close();
	m_vr.shutdown();
	m_menu.clear();
	m_vector.reset();
	renderer.clear();
	delete &renderer;
	for (Framebuffer& framebuffer : m_framebuffer)
	{
		framebuffer.remove();
	}
	glfwDestroyWindow(m_window);
	m_window = nullptr;
	glfwTerminate();

	delete &FontRenderer::instance();
	delete &ResourceManager::instance();

	return 0;
}
