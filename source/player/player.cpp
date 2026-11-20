// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define GL_GLEXT_PROTOTYPES

#include "player.h"
#include "application.h"
#include "opengl/render_manager.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <mpv/render_gl.h>
#include <GLFW/glfw3.h>

static const mpv_node* find_node(const mpv_node& node, const std::string& key)
{
	if ((node.format != MPV_FORMAT_NODE_MAP) || !node.u.list)
	{
		return nullptr;
	}

	for (int i = 0; i < node.u.list->num; i++)
	{
		if (key == node.u.list->keys[i])
		{
			return &node.u.list->values[i];
		}
	}
	return nullptr;
}

static std::string node_string(const mpv_node* node)
{
	if (node && (node->format == MPV_FORMAT_STRING) && node->u.string)
	{
		return node->u.string;
	}
	return "";
}

// Returns the address of the specified function (name) for the given context (ctx)
static void* get_proc_address(void* ctx __attribute__((unused)), const char* name)
{
	glfwGetCurrentContext();
	return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

Player::Player(void) :
	m_framebuffer(0),
	m_video_texture_id(0),
	m_video_textures{0, 0},
	m_texture_sizes{glm::uvec2(0, 0), glm::uvec2(0, 0)},
	m_display_texture_index(0),
	m_pending_texture_index(-1),
	m_render_fence(nullptr),
	m_texture_read_fences{nullptr, nullptr},
	m_playtime(0.0),
	m_duration(0.0),
	m_title(""),
	m_size(0, 0),
	m_playing(false),
	m_render_thread(),
	m_thread_running(false),
	m_wakeup(false),
	m_wakeup_mutex(),
	m_wakeup_cv(),
	m_window(nullptr),
	m_render_window(nullptr),
	m_context(mpv_create()),
	m_render(nullptr),
	m_initialized(false)
{
	if (!m_context)
	{
		throw std::runtime_error("failed creating context");
	}
}

Player::~Player(void)
{
	close();
}

void Player::thread_starter(Player* player)
{
	player->render_thread();
}

void Player::render_thread(void)
{
	glfwMakeContextCurrent(m_render_window);

	while (m_thread_running.load())
	{
		int target_index = 0;
		glm::uvec2 target_size(0, 0);
		GLsync read_fence = nullptr;
		{
			std::unique_lock<std::mutex> lk(m_wakeup_mutex);
			m_wakeup_cv.wait(lk, [this]{
				return (m_wakeup.load() && (m_pending_texture_index < 0)) || !m_thread_running.load();
			});

			if (!m_thread_running.load())
			{
				break;
			}

			m_wakeup.store(false);
			target_index = 1 - m_display_texture_index;
			target_size = m_size;
			read_fence = m_texture_read_fences[target_index];
			m_texture_read_fences[target_index] = nullptr;
		}

		if (read_fence)
		{
			glWaitSync(read_fence, 0, GL_TIMEOUT_IGNORED);
			glDeleteSync(read_fence);
		}

		if ((target_size.x > 0) && (target_size.y > 0))
		{
			if (m_framebuffer == 0)
			{
				glGenFramebuffers(1, &m_framebuffer);
			}

			if ((m_texture_sizes[target_index].x != target_size.x) ||
			    (m_texture_sizes[target_index].y != target_size.y))
			{
				glBindTexture(GL_TEXTURE_2D, m_video_textures[target_index]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
				             static_cast<GLsizei>(target_size.x), static_cast<GLsizei>(target_size.y),
				             0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				m_texture_sizes[target_index] = target_size;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
			                       m_video_textures[target_index], 0);

			if ((glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) &&
			    (mpv_render_context_update(m_render) & MPV_RENDER_UPDATE_FRAME))
			{
				mpv_opengl_fbo mpv_fbo{
					static_cast<int>(m_framebuffer),
					static_cast<int>(target_size.x),
					static_cast<int>(target_size.y),
					0
				};
				mpv_render_param params_fbo[] = {
					{MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo},
					{MPV_RENDER_PARAM_INVALID, nullptr}
				};

				// culling needs to be be disabled or only a black rectangle is rendered
				// glDisable(GL_CULL_FACE);
				mpv_render_context_render(m_render, params_fbo);
				// glEnable(GL_CULL_FACE);

				GLsync render_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
				glFlush();
				{
					std::lock_guard<std::mutex> lk(m_wakeup_mutex);
					m_pending_texture_index = target_index;
					m_render_fence = render_fence;
				}
			}
		}
	}

	glDeleteFramebuffers(1, &m_framebuffer);
	m_framebuffer = 0;
	glfwMakeContextCurrent(nullptr);
}

void Player::start_render_thread(void)
{
	if (m_thread_running.load())
	{
		return;
	}

	m_thread_running.store(true);
	m_render_thread = std::thread(thread_starter, this);
}

void Player::stop_render_thread(void)
{
	if (!m_thread_running.load())
	{
		return;
	}

	m_thread_running.store(false);
	m_wakeup_cv.notify_all();

	if (m_render_thread.joinable())
	{
		m_render_thread.join();
	}
}

void Player::on_mpv_events(void* ctx)
{
	Player* self = static_cast<Player*>(ctx);

	if (self)
	{
		self->handle_events();
	}
}

void Player::on_render_update(void* ctx)
{
	Player* self = static_cast<Player*>(ctx);

	if (self)
	{
		std::lock_guard<std::mutex> lk(self->m_wakeup_mutex);
		self->m_wakeup.store(true);
		self->m_wakeup_cv.notify_one();
	}
}

void Player::begin_frame(void)
{
	std::lock_guard<std::mutex> lk(m_wakeup_mutex);

	if ((m_pending_texture_index >= 0) && m_render_fence)
	{
		const GLenum status = glClientWaitSync(m_render_fence, 0, 0);

		if ((status == GL_ALREADY_SIGNALED) || (status == GL_CONDITION_SATISFIED))
		{
			m_display_texture_index = m_pending_texture_index;
			m_pending_texture_index = -1;
			glDeleteSync(m_render_fence);
			m_render_fence = nullptr;
			RenderManager::instance().texture(m_video_texture_id).set_video_target_id(
				m_video_textures[m_display_texture_index], m_size);
			m_wakeup_cv.notify_one();
		}
	}
}

void Player::end_frame(void)
{
	std::lock_guard<std::mutex> lk(m_wakeup_mutex);

	if (!m_video_textures[m_display_texture_index])
	{
		return;
	}

	GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush();
	GLsync old_fence = m_texture_read_fences[m_display_texture_index];
	m_texture_read_fences[m_display_texture_index] = fence;

	if (old_fence)
	{
		glDeleteSync(old_fence);
	}
}

void Player::handle_events(void)
{
	if (!m_context)
	{
		return;
	}

	/* handle all sorts of MPV events.
	 * A list of all properties is available at https://mpv.io/manual/master/#properties
	 * A list of all event types is available at https://mpv.io/manual/master/#list-of-events
	 * A list of all imput commands is available at https://mpv.io/manual/master/#list-of-input-commands
	 */
	bool more_events = true;

	while (more_events)
	{
		mpv_event* event = mpv_wait_event(m_context, 0);

		switch (event->event_id)
		{
			case MPV_EVENT_START_FILE:
				break;
			case MPV_EVENT_FILE_LOADED:
				break;
			case MPV_EVENT_END_FILE:
			{
				mpv_event_end_file* msg = static_cast<mpv_event_end_file*>(event->data);

				if (msg->reason == MPV_END_FILE_REASON_ERROR)
				{
					std::cout << "player thread stopped with error" << std::endl;
				}
				else if (msg->reason == MPV_END_FILE_REASON_EOF)
				{
					std::cout << "player reached end of file" << std::endl;
				}

				if ((msg->reason == MPV_END_FILE_REASON_EOF) ||
				    (msg->reason == MPV_END_FILE_REASON_ERROR))
				{
					m_playing = false;
				}
			}
			break;
			case MPV_EVENT_AUDIO_RECONFIG:
				break;
			case MPV_EVENT_VIDEO_RECONFIG:
			{
				int64_t width = 0;
				mpv_get_property(m_context, "width", MPV_FORMAT_INT64, &width);

				int64_t height = 0;
				mpv_get_property(m_context, "height", MPV_FORMAT_INT64, &height);

				{
					std::lock_guard<std::mutex> lk(m_wakeup_mutex);
					m_size.x = static_cast<GLuint>(width);
					m_size.y = static_cast<GLuint>(height);
					m_wakeup.store(true);
				}
				std::cout << "player video reconfiguration: " << m_size.x << " x " << m_size.y << std::endl;

				if (width && height)
				{
					Application::instance().projection().set_aspect(static_cast<float>(width) / static_cast<float>(height));
					Application::instance().update_projection();
				}
				m_wakeup_cv.notify_one();
			}
			break;
			case MPV_EVENT_PLAYBACK_RESTART:
				break;
			case MPV_EVENT_SET_PROPERTY_REPLY:

				if (event->error < 0)
				{
					std::cerr << "mpv pause property update failed: "
					          << mpv_error_string(event->error) << std::endl;
				}
				break;
			case MPV_EVENT_LOG_MESSAGE:
			{
				mpv_event_log_message* msg = static_cast<mpv_event_log_message*>(event->data);
				std::cerr << "mpv [" << msg->level << "] " << msg->prefix << ": " << msg->text;
			}
			break;
			case MPV_EVENT_PROPERTY_CHANGE:
			{
				mpv_event_property* prop = reinterpret_cast<mpv_event_property*>(event->data);
				const std::string name = prop->name;

				if (name == "time-pos")
				{
					if (mpv_get_property(m_context, prop->name, MPV_FORMAT_DOUBLE, &m_playtime) < 0)
					{
						// throw std::runtime_error("failed fetching playtime property");
					}
				}
				else if (name == "duration")
				{
					if (mpv_get_property(m_context, prop->name, MPV_FORMAT_DOUBLE, &m_duration) < 0)
					{
						// throw std::runtime_error("failed fetching duration property");
					}
					std::cout << "media duration: " << m_duration << std::endl;
				}
				else if (name == "media-title")
				{
					char* data = nullptr;

					if (mpv_get_property(m_context, prop->name, MPV_FORMAT_STRING, &data) >= 0)
					{
						m_title = data ? data : "";
					}
					mpv_free(data);
				}
				else
				{
					// std::cout << "property change: " << prop->name << std::endl;
					// std::cout << "property format: " << prop->format << std::endl;
				}
			}
			break;
			// case MPV_EVENT_SHUTDOWN:
			// m_quit = true;
			// break;
			case MPV_EVENT_IDLE:
			case MPV_EVENT_NONE:
				more_events = false;
				break;
			default:
				std::cout << "unhandled MPV event: " << event->event_id << std::endl;
				break;
		}
	}
}

void Player::set_option(const std::string& key, const std::string& value) const
{
	if (!m_context)
	{
		return;
	}
	const int status = mpv_set_option_string(m_context, key.c_str(), value.c_str());

	if (status < 0)
	{
		throw std::runtime_error(std::string("mpv API error: ") + mpv_error_string(status));
	}
}

void Player::open_file(const std::string& file_name, GLFWwindow* window, const size_t video_texture_id)
{
	m_window = window;
	m_video_texture_id = video_texture_id;
	glfwMakeContextCurrent(m_window);

	if (!m_context)
	{
		m_context = mpv_create();
	}

	if (!m_context)
	{
		throw std::runtime_error("failed creating context");
	}

	if (!m_initialized)
	{
		Texture& video_texture = RenderManager::instance().texture(m_video_texture_id);
		video_texture.init_video_target(glm::uvec2(1, 1), 0);
		m_video_textures[0] = video_texture.id();
		glGenTextures(1, &m_video_textures[1]);
		glBindTexture(GL_TEXTURE_2D, m_video_textures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
		m_display_texture_index = 0;
		m_pending_texture_index = -1;

		set_option("vd-lavc-dr", "no");  // no playback for some videos with vd-lavc-dr enabled
		set_option("vo", "libmpv");
		set_option("hwdec", "auto");
		set_option("profile", "gpu-hq");
		set_option("gpu-api", "opengl");
		set_option("vd-lavc-threads", "0");

		if (mpv_initialize(m_context) < MPV_ERROR_SUCCESS)
		{
			mpv_terminate_destroy(m_context);
			m_context = nullptr;
			throw std::runtime_error("Error: mpv_initialize failed");
		}

		mpv_opengl_init_params opengl_init_params = {
			get_proc_address,
			nullptr
		};
		int advanced_control = 1;
		mpv_render_param params[] = {
			{MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
			{MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &opengl_init_params},
			{MPV_RENDER_PARAM_ADVANCED_CONTROL, &advanced_control},
			{MPV_RENDER_PARAM_INVALID, nullptr}
		};

		if (mpv_render_context_create(&m_render, m_context, params) < 0)
		{
			mpv_terminate_destroy(m_context);
			m_context = nullptr;
			m_render = nullptr;
			throw std::runtime_error("Error: mpv_render_context_create failed");
		}

		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		m_render_window = glfwCreateWindow(1, 1, "Cine-VR render", nullptr, m_window);
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

		if (!m_render_window)
		{
			mpv_render_context_free(m_render);
			m_render = nullptr;
			mpv_terminate_destroy(m_context);
			m_context = nullptr;
			throw std::runtime_error("failed creating shared render context");
		}

		mpv_render_context_set_update_callback(m_render, on_render_update, this);

		mpv_observe_property(m_context, 0, "duration", MPV_FORMAT_DOUBLE);
		mpv_observe_property(m_context, 0, "time-pos", MPV_FORMAT_DOUBLE);
		mpv_observe_property(m_context, 0, "estimated-vf-fps", MPV_FORMAT_DOUBLE);
		mpv_observe_property(m_context, 0, "media-title", MPV_FORMAT_NONE);
		mpv_request_log_messages(m_context, "info");
		m_initialized = true;
		start_render_thread();
	}

	std::vector<const char*> cmd = {"loadfile", file_name.c_str(), "replace", nullptr};

	if (mpv_command(m_context, cmd.data()) < 0)
	{
		throw std::runtime_error("failed loading file: " + file_name);
	}

	play();
}

void Player::close(void)
{
	if (m_render)
	{
		mpv_render_context_set_update_callback(m_render, nullptr, nullptr);
	}

	stop_render_thread();

	if (m_render)
	{
		mpv_render_context_free(m_render);
		m_render = nullptr;
	}

	if (m_context)
	{
		mpv_terminate_destroy(m_context);
		m_context = nullptr;
	}

	if (m_render_window)
	{
		glfwDestroyWindow(m_render_window);
		m_render_window = nullptr;
	}

	if (m_video_textures[0] || m_video_textures[1])
	{
		glfwMakeContextCurrent(m_window);
		for (GLsync& fence : m_texture_read_fences)
		{
			if (fence)
			{
				glDeleteSync(fence);
				fence = nullptr;
			}
		}

		if (m_render_fence)
		{
			glDeleteSync(m_render_fence);
			m_render_fence = nullptr;
		}
		glDeleteTextures(2, m_video_textures);
		RenderManager::instance().texture(m_video_texture_id).set_video_target_id(0, glm::uvec2(0, 0));
		m_video_textures[0] = 0;
		m_video_textures[1] = 0;
	}

	m_initialized = false;
	m_playing = false;
	m_framebuffer = 0;
	m_size = glm::uvec2(0, 0);
	m_texture_sizes[0] = glm::uvec2(0, 0);
	m_texture_sizes[1] = glm::uvec2(0, 0);
	m_display_texture_index = 0;
	m_pending_texture_index = -1;
}

void Player::play(void)
{
	if (!m_context)
	{
		return;
	}

	int pause_value = 0;
	const int status = mpv_set_property_async(m_context, 1, "pause", MPV_FORMAT_FLAG, &pause_value);

	if (status < 0)
	{
		std::cerr << "failed requesting playback: " << mpv_error_string(status) << std::endl;
		return;
	}
	m_playing = true;
}

void Player::pause(void)
{
	if (!m_context)
	{
		return;
	}

	int pause_value = 1;
	const int status = mpv_set_property_async(m_context, 2, "pause", MPV_FORMAT_FLAG, &pause_value);

	if (status < 0)
	{
		std::cerr << "failed requesting pause: " << mpv_error_string(status) << std::endl;
		return;
	}
	m_playing = false;
}

void Player::stop(void)
{
	if (!m_context)
	{
		return;
	}
	mpv_set_property(m_context, "stop", MPV_FORMAT_STRING, nullptr);
	std::cout << "player stop" << std::endl;
	m_playing = false;
}

void Player::jump(const float step)
{
	if (!m_context)
	{
		return;
	}
	std::stringstream s;
	s << step;
	const std::string t = s.str();

	std::vector<const char*> cmd = { "seek", t.c_str(), nullptr };

	if (mpv_command(m_context, cmd.data()) < 0)
	{
		throw std::runtime_error("failed loading file.");
	}
}

bool Player::is_playing(void) const
{
	return m_playing;
}

float Player::duration(void) const
{
	return static_cast<float>(m_duration);
}

float Player::playtime(void) const
{
	return static_cast<float>(m_playtime);
}

void Player::bind(void) const
{
	RenderManager::instance().texture(m_video_texture_id).bind();
}

void Player::unbind(void) const
{
	RenderManager::instance().texture(m_video_texture_id).unbind();
}

float Player::volume(void) const
{
	double vol = 0.0;

	mpv_get_property(m_context, "ao-volume", MPV_FORMAT_DOUBLE, &vol);

	return static_cast<float>(vol);
}

std::vector<Player::track_t> Player::tracks_by_type(const std::string& type) const
{
	std::vector<track_t> result;

	if (!m_context)
	{
		return result;
	}

	mpv_node track_list = {};

	if ((mpv_get_property(m_context, "track-list", MPV_FORMAT_NODE, &track_list) < 0) ||
	    (track_list.format != MPV_FORMAT_NODE_ARRAY) || !track_list.u.list)
	{
		mpv_free_node_contents(&track_list);
		return result;
	}

	for (int i = 0; i < track_list.u.list->num; i++)
	{
		const mpv_node& entry = track_list.u.list->values[i];

		if (node_string(find_node(entry, "type")) != type)
		{
			continue;
		}

		const mpv_node* id = find_node(entry, "id");

		if (!id || (id->format != MPV_FORMAT_INT64))
		{
			continue;
		}

		const mpv_node* selected = find_node(entry, "selected");
		result.push_back({
			id->u.int64,
			node_string(find_node(entry, "title")),
			node_string(find_node(entry, "lang")),
			selected && (selected->format == MPV_FORMAT_FLAG) && (selected->u.flag != 0)
		});
	}

	mpv_free_node_contents(&track_list);
	return result;
}

std::vector<Player::track_t> Player::audio_tracks(void) const
{
	return tracks_by_type("audio");
}

std::vector<Player::track_t> Player::subtitle_tracks(void) const
{
	return tracks_by_type("sub");
}

void Player::set_audio_track(const int64_t id)
{
	if (!m_context)
	{
		return;
	}
	const std::string value = std::to_string(id);
	const int status = mpv_set_property_string(m_context, "aid", value.c_str());

	if (status < 0)
	{
		throw std::runtime_error(std::string("mpv API error: ") + mpv_error_string(status));
	}
}

void Player::set_subtitle_track(const int64_t id)
{
	if (!m_context)
	{
		return;
	}
	const std::string value = (id < 0) ? "no" : std::to_string(id);
	const int status = mpv_set_property_string(m_context, "sid", value.c_str());

	if (status < 0)
	{
		throw std::runtime_error(std::string("mpv API error: ") + mpv_error_string(status));
	}
}

void Player::set_volume(const float vol)
{
	double vold = static_cast<double>(vol);

	mpv_set_property(m_context, "ao-volume", MPV_FORMAT_DOUBLE, reinterpret_cast<void*>(&vold));
}
