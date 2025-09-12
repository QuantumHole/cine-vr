#include "openvr_interface.h"
#include <stdexcept>

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
	m_system(nullptr),
	m_compositor(nullptr)
{
}

OpenVRInterface::~OpenVRInterface(void)
{
	vr::VR_Shutdown();
}

void OpenVRInterface::init(void)
{
	vr::EVRInitError eError = vr::VRInitError_None;

	m_system = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		throw std::runtime_error(std::string("VR_Init failed: ") + vr::VR_GetVRInitErrorAsEnglishDescription(eError));
	}

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
