// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PLAYER_H
#define PLAYER_H

#include <mpv/client.h>
#include <glm/glm.hpp>
#include <GL/gl.h>
#include <string>

class Player
{
	private:
		static bool m_wakeup;

		GLuint m_framebuffer;
		GLuint m_video_texture;
		double m_playtime;
		double m_duration;
		std::string m_title;
		glm::uvec2 m_size;

		struct mpv_handle* m_context;
		struct mpv_render_context* m_render;

		static void on_mpv_events(void* ctx);
		static void on_render_update(void* ctx);

		Player(const Player&);
		Player& operator=(const Player&);

		void set_option(const std::string& key, const std::string& value) const;

	public:
		explicit Player(void);
		~Player(void);

		void open_file(const std::string& file_name);
		void close(void);
		void play(void);
		void pause(void);
		void stop(void);
		void jump(const float step);
		float playtime(void) const;

		void handle_events(void);
		void bind(void) const;
		void unbind(void) const;
};

#endif
