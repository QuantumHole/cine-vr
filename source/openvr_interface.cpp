// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "openvr_interface.h"
#include <stdexcept>
#include <fstream>
#include <unistd.h>
#include <iostream>

// Convert OpenVR matrix to glm
static glm::mat4 ConvertSteamVRMatrixToGLMMat(const vr::HmdMatrix34_t& matPose)
{
	glm::mat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
		);

	return matrixObj;
}

static glm::mat4 ConvertSteamVRMatrixToGLMMat(const vr::HmdMatrix44_t& mat)
{
	glm::mat4 m;

	for (int r = 0; r < 4; ++r)
	{
		for (int c = 0; c < 4; ++c)
		{
			m[c][r] = mat.m[r][c];
		}
	}
	return m;
}

OpenVRInterface::OpenVRInterface(void) :
	m_clip_near(0.1f),
	m_clip_far(100.0f),
	m_poses(vr::k_unMaxTrackedDeviceCount),
	m_actionset(vr::k_ulInvalidActionSetHandle),
	m_input_handle(),
	m_system(nullptr),
	m_input(nullptr),
	m_compositor(nullptr)
{
}

OpenVRInterface::~OpenVRInterface(void)
{
	vr::VR_Shutdown();
}

void OpenVRInterface::initActionHandle(const input_action_t input, const std::string& path)
{
	vr::EVRInputError status = vr::VRInput()->GetActionHandle(path.c_str(), &m_input_handle[input]);

	if (status != vr::VRInputError_None)
	{
		throw std::runtime_error("failed importing action handle '" + path + "': " + std::to_string(status));
	}
}

void OpenVRInterface::init(void)
{
	vr::EVRInitError initstatus = vr::VRInitError_None;

	m_system = vr::VR_Init(&initstatus, vr::VRApplication_Scene);

	if (initstatus != vr::VRInitError_None)
	{
		throw std::runtime_error(std::string("VR_Init failed: ") + vr::VR_GetVRInitErrorAsEnglishDescription(initstatus));
	}

	m_input = vr::VRInput();

	if (!m_input)
	{
		throw std::runtime_error("Fehler beim Abrufen des IVRInput-Interfaces.");
	}

	char action_manifest_path[PATH_MAX];
	realpath("actions/controller.json", action_manifest_path);

	if (access(action_manifest_path, F_OK) == -1)
	{
		throw std::runtime_error("Unable to find controller.json!");
	}

	std::cerr << "Using openvr config file: " << action_manifest_path << std::endl;

	vr::EVRInputError status = vr::VRInput()->SetActionManifestPath(action_manifest_path);

	if (status != vr::VRInputError_None)
	{
		throw std::runtime_error("failed importing action manifest: " + std::to_string(status));
	}

	status = vr::VRInput()->GetActionSetHandle("/actions/cinevr", &m_actionset);

	if (status != vr::VRInputError_None)
	{
		throw std::runtime_error("failed importing action set: " + std::to_string(status));
	}

	initActionHandle(ACTION_PADCLICK, "/actions/cinevr/in/PadClick");
	initActionHandle(ACTION_GRIP, "/actions/cinevr/in/Grip");
	initActionHandle(ACTION_TRIGGER, "/actions/cinevr/in/Trigger");
	initActionHandle(ACTION_MENU, "/actions/cinevr/in/Menu");
	initActionHandle(ACTION_SYSTEM, "/actions/cinevr/in/System");
	initActionHandle(ACTION_TRIGGER_VALUE, "/actions/cinevr/in/TriggerValue");
	initActionHandle(ACTION_ANALOG, "/actions/cinevr/in/AnalogInput");
	initActionHandle(ACTION_HAPTIC_LEFT, "/actions/cinevr/out/haptic_left");
	initActionHandle(ACTION_HAPTIC_RIGHT, "/actions/cinevr/out/haptic_right");

	// Ensure compositor present
	// try to init compositor explicitly
	m_compositor = vr::VRCompositor();

	if (!m_compositor)
	{
		throw std::runtime_error("failed initializing IVRCompositor");
	}
}

void OpenVRInterface::read_poses(void)
{
	// Get poses (WaitGetPoses also updates compositor)
	vr::VRCompositor()->WaitGetPoses(m_poses.data(), static_cast<uint32_t>(m_poses.size()), nullptr, 0);
}

glm::uvec2 OpenVRInterface::render_target_size(void) const
{
	// Get recommended render target size
	uint32_t renderWidth = 0;
	uint32_t renderHeight = 0;

	m_system->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
	return glm::uvec2(renderWidth, renderHeight);
}

std::set<vr::TrackedDeviceIndex_t> OpenVRInterface::devices(void) const
{
	std::set<vr::TrackedDeviceIndex_t> devices;

	for (vr::TrackedDeviceIndex_t i = 0; i < m_poses.size(); i++)
	{
		if (m_poses[i].bPoseIsValid)
		{
			devices.insert(i);
		}
	}
	return devices;
}

std::string OpenVRInterface::name(const vr::TrackedDeviceIndex_t device) const
{
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(device, vr::Prop_RenderModelName_String, nullptr, 0, nullptr);

	if (unRequiredBufferLen == 0)
	{
		return "";
	}

	char* pchBuffer = new char[ unRequiredBufferLen ];

	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(device, vr::Prop_RenderModelName_String, pchBuffer, unRequiredBufferLen, nullptr);
	std::string name = pchBuffer;

	delete [] pchBuffer;
	return name;
}

glm::mat4 OpenVRInterface::projection(const vr::Hmd_Eye eye) const
{
	vr::HmdMatrix44_t proj = m_system->GetProjectionMatrix(eye, m_clip_near, m_clip_far);

	return ConvertSteamVRMatrixToGLMMat(proj);
}

glm::mat4 OpenVRInterface::view(const vr::Hmd_Eye eye) const
{
	glm::mat4 hmdPose = pose(vr::k_unTrackedDeviceIndex_Hmd);

	vr::HmdMatrix34_t eye2head = m_system->GetEyeToHeadTransform(eye);
	glm::mat4 eyeToHead = ConvertSteamVRMatrixToGLMMat(eye2head);

	glm::mat4 view = glm::inverse(hmdPose * eyeToHead);

	return view;
}

glm::mat4 OpenVRInterface::pose(vr::TrackedDeviceIndex_t device) const
{
	// Get HMD pose
	glm::mat4 pose = glm::mat4(1.0f);

	if (m_poses.at(device).bPoseIsValid)
	{
		pose = ConvertSteamVRMatrixToGLMMat(m_poses[device].mDeviceToAbsoluteTracking);
	}
	return pose;
}

vr::VRControllerState_t OpenVRInterface::controller_state(vr::TrackedDeviceIndex_t device) const
{
	// handle controller input: check trigger press
	vr::VRControllerState_t state;

	bool status = m_system->GetControllerState(device, &state, sizeof(state));

	if (!status)
	{
		memset(&state, 0, sizeof(state));
	}
	return state;
}

vr::ETrackedDeviceClass OpenVRInterface::device_class(const vr::TrackedDeviceIndex_t device) const
{
	return m_system->GetTrackedDeviceClass(device);
}

void OpenVRInterface::submit(const vr::Hmd_Eye eye, const GLuint texture_id) const
{
	vr::Texture_t texture = {reinterpret_cast<void*>(static_cast<uintptr_t>(texture_id)), vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
	vr::EVRCompositorError compErr = m_compositor->Submit(eye, &texture);

	if (compErr != 0)
	{
	}
}

void OpenVRInterface::handoff(void) const
{
	m_compositor->PostPresentHandoff();
}

void OpenVRInterface::update(void) const
{
	vr::VRActiveActionSet_t actionSet = { m_actionset, vr::k_ulInvalidInputValueHandle, 0, 0, 0 };
	vr::EVRInputError error = vr::VRInput()->UpdateActionState(&actionSet, sizeof(vr::VRActiveActionSet_t), 1);

	if (error != vr::VRInputError_None)
	{
		throw std::runtime_error("failed updating action status: " + std::to_string(error));
	}
}

bool OpenVRInterface::getButtonAction(const input_action_t action, const bool debounce) const
{
	std::map<input_action_t, vr::VRInputValueHandle_t>::const_iterator iter = m_input_handle.find(action);

	if (iter == m_input_handle.end())
	{
		throw std::runtime_error("invalid input action");
	}

	vr::InputDigitalActionData_t data;
	vr::EVRInputError error = vr::VRInput()->GetDigitalActionData(iter->second, &data, sizeof(data), vr::k_ulInvalidInputValueHandle);

	if (error != vr::VRInputError_None)
	{
		throw std::runtime_error("failed reading digital action status: " + std::to_string(error));
	}

	return data.bActive && data.bState && ((debounce && data.bChanged) || !debounce);
}

glm::vec3 OpenVRInterface::getButtonPosition(const input_action_t action) const
{
	std::map<input_action_t, vr::VRInputValueHandle_t>::const_iterator iter = m_input_handle.find(action);

	if (iter == m_input_handle.end())
	{
		throw std::runtime_error("invalid input action");
	}

	vr::InputAnalogActionData_t data;
	vr::EVRInputError error = vr::VRInput()->GetAnalogActionData(iter->second, &data, sizeof(data), vr::k_ulInvalidInputValueHandle);

	if (error != vr::VRInputError_None)
	{
		throw std::runtime_error("failed reading analog action status: " + std::to_string(error));
	}

	if (data.bActive)
	{
		return glm::vec3(data.x, data.y, data.z);
	}

	return glm::vec3(0.0f, 0.0f, 0.0f);
}

void OpenVRInterface::haptic(const input_action_t action) const
{
	std::map<input_action_t, vr::VRInputValueHandle_t>::const_iterator iter = m_input_handle.find(action);

	if (iter == m_input_handle.end())
	{
		throw std::runtime_error("invalid input action");
	}

	const float start_time = 0.0f;
	const float duration = 0.1f;
	const float frequency = 4.0f;
	const float amplitude = 0.5f;
	vr::VRInput()->TriggerHapticVibrationAction(iter->second, start_time, duration, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
}
