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

class OpenVRInterface
{
	private:
		const float m_clip_near;
		const float m_clip_far;
		std::vector<vr::TrackedDevicePose_t> m_poses;

		vr::IVRSystem* m_system;
		vr::IVRCompositor* m_compositor;

		OpenVRInterface(const OpenVRInterface&);
		OpenVRInterface operator=(const OpenVRInterface&);

	public:
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
};

#endif
