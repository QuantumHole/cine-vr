// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PLAYER_H
#define PLAYER_H

#include <mpv/client.h>
#include <glm/glm.hpp>
#include <GL/gl.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <GLFW/glfw3.h>

class Player
{
	private:
		GLuint m_framebuffer;
		GLuint m_video_texture;
		double m_playtime;
		double m_duration;
		std::string m_title;
		glm::uvec2 m_size;
		bool m_playing;

		std::thread m_render_thread;
		std::atomic<bool> m_thread_running;
		std::atomic<bool> m_wakeup;
		std::mutex m_wakeup_mutex;
		std::condition_variable m_wakeup_cv;

		GLFWwindow* m_window;
		struct mpv_handle* m_context;
		struct mpv_render_context* m_render;

		void render_thread(void);
		static void thread_starter(Player* player);
		static void on_mpv_events(void* ctx);
		static void on_render_update(void* ctx);

		Player(const Player&);
		Player& operator=(const Player&);
		void start_render_thread(void);
		void stop_render_thread(void);
		void render_frame(void);

		void set_option(const std::string& key, const std::string& value) const;

	public:
		explicit Player(void);
		~Player(void);

		void open_file(const std::string& file_name, GLFWwindow* window);
		void close(void);
		void play(void);
		void pause(void);
		void stop(void);
		void jump(const float step);
		void set_volume(const float vol);
		bool is_playing(void) const;
		float duration(void) const;
		float playtime(void) const;
		float volume(void) const;
		void handle_events(void);

		void bind(void) const;
		void unbind(void) const;
};

#endif
