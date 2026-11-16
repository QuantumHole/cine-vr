// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PLAYER_H
#define PLAYER_H

#include <mpv/client.h>
#include <glm/glm.hpp>
#include <GL/gl.h>
#include <cstdint>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <GLFW/glfw3.h>
#include <vector>

class Player
{
	public:
		struct track_t
		{
			int64_t id;
			std::string title;
			std::string language;
			bool selected;
		};

	private:
		GLuint m_framebuffer;
		size_t m_video_texture_id;
		GLuint m_video_textures[2];
		glm::uvec2 m_texture_sizes[2];
		int m_display_texture_index;
		int m_pending_texture_index;
		GLsync m_render_fence;
		GLsync m_texture_read_fences[2];
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
		GLFWwindow* m_render_window;
		struct mpv_handle* m_context;
		struct mpv_render_context* m_render;
		bool m_initialized;

		void render_thread(void);
		static void thread_starter(Player* player);
		static void on_mpv_events(void* ctx);
		static void on_render_update(void* ctx);

		Player(const Player&);
		Player& operator=(const Player&);
		void start_render_thread(void);
		void stop_render_thread(void);

		void set_option(const std::string& key, const std::string& value) const;
		std::vector<track_t> tracks_by_type(const std::string& type) const;

	public:
		explicit Player(void);
		~Player(void);

		void open_file(const std::string& file_name, GLFWwindow* window, const size_t video_texture_id);
		void close(void);
		void play(void);
		void pause(void);
		void stop(void);
		void begin_frame(void);
		void end_frame(void);
		void jump(const float step);
		void set_volume(const float vol);
		bool is_playing(void) const;
		float duration(void) const;
		float playtime(void) const;
		float volume(void) const;
		std::vector<track_t> audio_tracks(void) const;
		std::vector<track_t> subtitle_tracks(void) const;
		void set_audio_track(const int64_t id);
		void set_subtitle_track(const int64_t id);
		void handle_events(void);

		void bind(void) const;
		void unbind(void) const;
};

#endif
