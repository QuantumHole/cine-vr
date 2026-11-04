// SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPLICATION_H
#define APPLICATION_H

#define GL_GLEXT_PROTOTYPES

#include "opengl/framebuffer.h"
#include "util/openvr_interface.h"
#include "util/enum_iterator.h"
#include "opengl/render_manager.h"
#include "gui/controller.h"
#include "gui/menu.h"
#include "gui/graphics_svg.h"
#include "gui/projection.h"
#include "player/player.h"

#include <GLFW/glfw3.h>

/** main application class */
class Application
{
	private:
		/** type of the currently displayed content. */
		typedef enum
		{
			SOURCE_NONE,     ///< no content is displayed
			SOURCE_IMAGE,    ///< an image is displayed
			SOURCE_VIDEO,    ///< a video is displayed
			SOURCE_VECTOR    ///< a vector graphic is displayed
		}
		source_t;

		static Application* m_instance;         ///< singleton instance of the application

		bool g_Running;                         ///< indicates if the application is running
		GLFWwindow* g_window;                   ///< GLFW window
		OpenVRInterface g_vr;                   ///< OpenVR interface functions
		size_t g_scene_shader;                  ///< ID of the current shader set for the scene
		size_t g_image_texture;                 ///< ID of the displayed image
		size_t g_video_texture;                 ///< ID of the displayed video
		size_t g_scene_shape_set;               ///< ID of the shape set for the scene
		RenderManager::RenderableId g_scene_renderable;   ///< ID of the renderable compound for the scene
		bool g_scene_ready;                     ///< indicates if the scene is ready
		std::map<vr::TrackedDeviceIndex_t, Controller> g_controller;   ///< map of OpenVR controllers
		std::vector<Framebuffer> g_framebuffer; ///< vector of framebuffers for each eye
		Menu g_menu;                            ///< menu for user interaction
		Projection g_projection;                ///< projection settings for the scene
		std::unique_ptr<GraphicsSVG> g_vector;  ///< currently displayed vector graphic
		glm::vec3 g_hmd_reference_pos;          ///< reference position of the HMD
		glm::quat g_hmd_reference_rot;          ///< reference rotation of the HMD
		std::string g_current_file_name;        ///< name of the currently displayed file
		const float g_jump_step;                ///< jump step size for player controls
		source_t g_source;                      ///< type of the currently displayed content
		Player g_player;                        ///< player for video playback
		static glm::uvec2 g_window_size;               ///< size of the application window

		static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

		/** private constructor. */
		Application(void);

		/** private copy constructor. */
		Application(const Application&) = delete;

		/** private assignment operator. */
		Application& operator=(const Application&) = delete;

		void reset_reference(void);

		std::string make_absolute(const std::string& relative);
		std::string file_step(const int32_t step);

		void setup_shader(const ShaderSet& shader, const vr::Hmd_Eye eye);
		void setup_hmd(const glm::mat4& hmdPose);

	public:
		/** returns the singleton instance of the application.
		 * @return reference to the application instance
		 */
		static Application& instance(void);

		/** destructor. */
		~Application(void);

		/** runs the application. */
		int run(void);

		/** quits the application. */
		void quit(void);

		/** jumps backward in the player. */
		void player_backward(void);

		/** jumps forward in the player. */
		void player_forward(void);

		/** pauses the player. */
		void player_pause(void);

		/** toggles between play and pause for the player. */
		void player_play(void);

		/** plays the previous file in the player. */
		void player_previous(void);

		/** plays the next file in the player. */
		void player_next(void);

		/** opens a file in the player.
		 * @param file_name path to the file to open
		 */
		void player_open_file(const std::string& file_name);

		/** shows the desktop in the player. */
		void player_show_desktop(void);

		/** returns a reference to the player.
		 * @return reference to the player
		 */
		Player& player(void);

		/** returns a reference to the projection.
		 * @return reference to the projection
		 */
		Projection& projection(void);

		/** returns a reference to the shader set.
		 * @return reference to the shader set
		 */
		ShaderSet& shader(void);

		/** updates the projection settings. */
		void update_projection(void);
};

#endif
