// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define GL_GLEXT_PROTOTYPES

#include "player.h"
#include "main.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <mpv/render_gl.h>
#include <GLFW/glfw3.h>

// Returns the address of the specified function (name) for the given context (ctx)
static void* get_proc_address(void* ctx __attribute__((unused)), const char* name)
{
	glfwGetCurrentContext();
	return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

Player::Player(void) :
	m_framebuffer(0),
	m_video_texture(0),
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
	m_context(mpv_create()),
	m_render(nullptr)
{
	if (!m_context)
	{
		throw std::runtime_error("failed creating context");
	}
}

Player::~Player(void)
{
	if (m_context)
	{
		mpv_render_context_set_update_callback(m_render, nullptr, nullptr);
		mpv_terminate_destroy(m_context);
	}
	m_context = nullptr;
}

void Player::thread_starter(Player* player)
{
	player->render_thread();
}

void Player::render_thread(void)
{
	glfwMakeContextCurrent(glfwGetCurrentContext());

	while (m_thread_running.load())
	{
		{
			std::unique_lock<std::mutex> lk(m_wakeup_mutex);
			m_wakeup_cv.wait(lk, [this]{
				return m_wakeup.load() || !m_thread_running.load();
			});
			m_wakeup.store(false);
		}

		if (!m_thread_running.load())
		{
			break;
		}

		if (mpv_render_context_update(m_render) & MPV_RENDER_UPDATE_FRAME)
		{
			mpv_opengl_fbo mpv_fbo{
				static_cast<int>(m_framebuffer),
				static_cast<int>(m_size.x),
				static_cast<int>(m_size.y),
				0
			};
			mpv_render_param params_fbo[] = {
				{MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo},
				{MPV_RENDER_PARAM_INVALID, nullptr}
			};

			glDisable(GL_CULL_FACE);
			mpv_render_context_render(m_render, params_fbo);
			glEnable(GL_CULL_FACE);
		}
	}
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
		self->m_wakeup.store(true);
		self->m_wakeup_cv.notify_one();
	}
}

void Player::render_frame(void)
{
	// glfwMakeContextCurrent(glfwGetCurrentContext());
	glfwMakeContextCurrent(m_window);

	if ((mpv_render_context_update(m_render) & MPV_RENDER_UPDATE_FRAME))
	{
		mpv_opengl_fbo mpv_fbo = {
			static_cast<int>(m_framebuffer),
			static_cast<int>(m_size.x),
			static_cast<int>(m_size.y),
			0
		};

		mpv_render_param params_fbo[] = {
			{MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo},
			{MPV_RENDER_PARAM_INVALID, nullptr}
		};

		glDisable(GL_CULL_FACE);          // culling needs to be be disabled or only a black rectangle is rendered
		mpv_render_context_render(m_render, params_fbo);
		glEnable(GL_CULL_FACE);
	}

	m_wakeup.store(false);
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
				mpv_render_context_set_update_callback(m_render, nullptr, nullptr);
				mpv_terminate_destroy(m_context);
				m_context = nullptr;
				return;
			}
			case MPV_EVENT_AUDIO_RECONFIG:
				break;
			case MPV_EVENT_VIDEO_RECONFIG:
			{
				int64_t width = 0;
				mpv_get_property(m_context, "width", MPV_FORMAT_INT64, &width);

				int64_t height = 0;
				mpv_get_property(m_context, "height", MPV_FORMAT_INT64, &height);

				m_size.x = static_cast<GLuint>(width);
				m_size.y = static_cast<GLuint>(height);
				std::cout << "player video reconfiguration: " << m_size.x << " x " << m_size.y << std::endl;

				if (width && height)
				{
					// glfwMakeContextCurrent(glfwGetCurrentContext());
					glfwMakeContextCurrent(m_window);

					// Framebuffer for Video Target - Video Texture
					glGenFramebuffers(1, &m_framebuffer);
					glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
					// create a color attachment texture
					glGenTextures(1, &m_video_texture);
					glBindTexture(GL_TEXTURE_2D, m_video_texture);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(m_size.x), static_cast<GLsizei>(m_size.y), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_video_texture, 0);

					if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
					{
						std::cout << "failed creating framebuffer" << std::endl;
					}
					projection().set_aspect(static_cast<float>(width) / static_cast<float>(height));
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			break;
			case MPV_EVENT_PLAYBACK_RESTART:
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
					const char* data = nullptr;

					if (mpv_get_property(m_context, prop->name, MPV_FORMAT_STRING, &data) < 0)
					{
					}
					m_title = data;
				}
				else
				{
					std::cout << "property change: " << prop->name << std::endl;
					std::cout << "property format: " << prop->format << std::endl;
				}
			}
			break;
// case MPV_EVENT_SHUTDOWN:
// m_quit = true;
// break;
// case MPV_EVENT_LOG_MESSAGE:
// {
// mpv_event_log_message* msg = static_cast<mpv_event_log_message*>(event->data);
// printf("[%s] %s: %s", msg->prefix, msg->level, msg->text);
// }
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

	if (m_wakeup.load())
	{
		render_frame();
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

void Player::open_file(const std::string& file_name, GLFWwindow* window)
{
	m_window = window;

	if (!m_context)
	{
		m_context = mpv_create();
	}

	if (!m_context)
	{
		throw std::runtime_error("failed creating context");
	}

	if (mpv_initialize(m_context) < MPV_ERROR_SUCCESS)
	{
		mpv_destroy(m_context);
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
		{MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME, nullptr},
		{ MPV_RENDER_PARAM_INVALID, nullptr }
	};

	if (mpv_render_context_create(&m_render, m_context, params) < 0)
	{
		mpv_render_context_set_update_callback(m_render, nullptr, nullptr);
		mpv_destroy(m_context);
		m_context = nullptr;
		m_render = nullptr;
		throw std::runtime_error("Error: mpv_render_context_create failed");
	}

	// mpv_set_wakeup_callback(m_context, on_mpv_events, this);
	mpv_render_context_set_update_callback(m_render, on_render_update, this);

	set_option("vd-lavc-dr", "yes");
	set_option("vo", "libmpv");
	set_option("hwdec", "auto");
	set_option("profile", "gpu-hq");
	set_option("gpu-api", "opengl");
	set_option("vd-lavc-threads", "0");
	// set_option("force-window", "immediate");
	// set_option("cache", "yes");
	// set_option("cache-pause", "no");

	mpv_observe_property(m_context, 0, "duration", MPV_FORMAT_DOUBLE);
	mpv_observe_property(m_context, 0, "time-pos", MPV_FORMAT_DOUBLE);
	mpv_observe_property(m_context, 0, "estimated-vf-fps", MPV_FORMAT_DOUBLE);
	mpv_observe_property(m_context, 0, "media-title", MPV_FORMAT_NONE);

	std::vector<const char*> cmd = {"loadfile", file_name.c_str(), nullptr};

	if (mpv_command(m_context, cmd.data()) < 0)
	{
		throw std::runtime_error("failed loading file: " + file_name);
	}

	play();
}

void Player::close(void)
{
	pause();

	std::cout << "player close" << std::endl;
}

void Player::play(void)
{
	if (!m_context)
	{
		return;
	}

	int pause_value = 0;
	mpv_set_property(m_context, "pause", MPV_FORMAT_FLAG, &pause_value);
	m_playing = true;
}

void Player::pause(void)
{
	if (!m_context)
	{
		return;
	}

	int pause_value = 1;
	mpv_set_property(m_context, "pause", MPV_FORMAT_FLAG, &pause_value);
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
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_video_texture);
}

void Player::unbind(void) const
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

float Player::volume(void) const
{
	double vol = 0.0;

	mpv_get_property(m_context, "ao-volume", MPV_FORMAT_DOUBLE, &vol);

	return static_cast<float>(vol);
}

void Player::set_volume(const float vol)
{
	double vold = static_cast<double>(vol);

	mpv_set_property(m_context, "ao-volume", MPV_FORMAT_DOUBLE, reinterpret_cast<void*>(&vold));
}
