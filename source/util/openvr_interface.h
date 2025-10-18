// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef OPENVR_INTERFACE_H
#define OPENVR_INTERFACE_H

#include <GL/gl.h>
#include <openvr.h>

// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <set>
#include <vector>
#include <map>

class OpenVRInterface
{
	public:
		typedef enum
		{
			INPUT_GRIP,
			INPUT_MENU,
			INPUT_SYSTEM,
			INPUT_PADCLICK,
			INPUT_ANALOG,
			INPUT_TRIGGER,
			INPUT_TRIGGER_VALUE,
			HAPTIC_LEFT,
			HAPTIC_RIGHT
		}
		input_action_t;

		typedef struct
		{
			bool pressed;
			bool released;
		}
		button_state_t;

		typedef struct
		{
			button_state_t button;
			float value;
		}
		throttle_state_t;

		typedef struct
		{
			button_state_t button;
			bool touched;
			glm::vec2 position;
		}
		pad_state_t;

		typedef struct
		{
			button_state_t system;
			button_state_t menu;
			button_state_t grip;
			throttle_state_t trigger;
			pad_state_t pad;
		}
		input_state_t;

		OpenVRInterface(void);
		~OpenVRInterface(void);

		void init(void);
		glm::uvec2 render_target_size(void) const;
		void read_poses(void);
		std::set<vr::TrackedDeviceIndex_t> devices(void) const;
		std::string name(const vr::TrackedDeviceIndex_t device) const;
		glm::mat4 projection(const vr::Hmd_Eye eye) const;
		glm::mat4 view(const vr::Hmd_Eye eye) const;
		glm::mat4 pose(const vr::TrackedDeviceIndex_t device) const;
		vr::VRControllerState_t controller_state(const vr::TrackedDeviceIndex_t device) const;
		vr::ETrackedDeviceClass device_class(const vr::TrackedDeviceIndex_t device) const;
		void submit(const vr::Hmd_Eye eye, const GLuint texture_id) const;
		void handoff(void) const;

		void update(void) const;
		bool getButtonAction(const input_action_t action, const bool debounce = true) const;
		glm::vec3 getButtonPosition(const input_action_t action) const;
		void haptic(const input_action_t input) const;
		float battery(const vr::TrackedDeviceIndex_t device) const;
		const input_state_t& read_input(void);

	private:
		const float m_clip_near;
		const float m_clip_far;
		input_state_t m_input_state;
		std::vector<vr::TrackedDevicePose_t> m_poses;
		vr::VRActionSetHandle_t m_actionset;
		std::map<input_action_t, vr::VRInputValueHandle_t> m_input_handle;

		vr::IVRSystem* m_system;
		vr::IVRInput* m_input;
		vr::IVRCompositor* m_compositor;

		OpenVRInterface(const OpenVRInterface&);
		OpenVRInterface operator=(const OpenVRInterface&);

		void initActionHandle(const input_action_t input, const std::string& path);
};

#endif
