// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <openvr.h>

// #define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static bool g_Running = true;
static GLFWwindow* g_Window = nullptr;
static vr::IVRSystem* g_VRSystem = nullptr;
static vr::IVRCompositor* g_VRCompositor = nullptr;

struct GLTexture
{
	GLuint tex;
	int width, height;
};

struct FramebufferDesc
{
	GLuint m_nRenderFramebufferId;
	GLuint m_nRenderTextureId;
	GLuint m_nDepthBufferId;
	int m_nRenderWidth;
	int m_nRenderHeight;
};

// Render a rectangle (centered at origin, size 1x1 in XY plane)
struct Mesh
{
	GLuint vao = 0, vbo = 0, ebo = 0;
	int idxCount = 0;
};

Mesh g_RectMesh;
Mesh g_LineMesh;
Mesh g_PointMesh;
GLuint g_Prog = 0;

static FramebufferDesc leftEyeDesc, rightEyeDesc;

// helper: print VR errors
static void PrintVRError(const std::string& prefix, vr::EVRInitError err)
{
	if (err != vr::VRInitError_None)
	{
		std::cerr << prefix << ": " << vr::VR_GetVRInitErrorAsEnglishDescription(err) << "\n";
	}
}

// shader helpers (very simple)
static GLuint CompileShader(const char* src, GLenum type)
{
	GLuint s = glCreateShader(type);

	glShaderSource(s, 1, &src, NULL);
	glCompileShader(s);
	GLint ok;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);

	if (!ok)
	{
		char buf[1024];
		glGetShaderInfoLog(s, sizeof(buf), NULL, buf);
		std::cerr << "Shader compile error: " << buf << "\n";
		return 0;
	}
	return s;
}

static GLuint CreateSimpleProg()
{
	const char* vs =
		"#version 330 core\n"
		"layout(location=0) in vec3 pos;\n"
		"layout(location=1) in vec3 color;\n"
		"uniform mat4 u_mvp;\n"
		"out vec3 vColor;\n"
		"void main(){ vColor=color; gl_Position=u_mvp*vec4(pos,1.0); }\n";

	const char* fs =
		"#version 330 core\n"
		"in vec3 vColor;\n"
		"out vec4 outColor;\n"
		"void main(){ outColor=vec4(vColor,1.0); }\n";

	GLuint vsid = CompileShader(vs, GL_VERTEX_SHADER);
	GLuint fsid = CompileShader(fs, GL_FRAGMENT_SHADER);
	GLuint prog = glCreateProgram();

	glAttachShader(prog, vsid);
	glAttachShader(prog, fsid);
	glLinkProgram(prog);
	GLint ok;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);

	if (!ok)
	{
		char buf[1024];
		glGetProgramInfoLog(prog, sizeof(buf), NULL, buf);
		std::cerr << "Program link error: " << buf << "\n";
		return 0;
	}
	glDeleteShader(vsid);
	glDeleteShader(fsid);
	return prog;
}

// Create framebuffer for eye
static bool CreateFrameBuffer(int width, int height, FramebufferDesc& framebufferDesc)
{
	framebufferDesc.m_nRenderWidth = width;
	framebufferDesc.m_nRenderHeight = height;
	glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nRenderTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferDesc.m_nRenderWidth, framebufferDesc.m_nRenderHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nRenderTextureId, 0);

	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, framebufferDesc.m_nRenderWidth, framebufferDesc.m_nRenderHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "Framebuffer incomplete: " << status << "\n";
		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

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

// Get eye projection & eye-to-head transform
static glm::mat4 GetHMDProjection(vr::Hmd_Eye eye, float nearClip, float farClip)
{
	vr::HmdMatrix44_t proj = g_VRSystem->GetProjectionMatrix(eye, nearClip, farClip);

	return ConvertSteamVRMatrixToGLMMat(proj);
}

static glm::mat4 GetHMDEyeToHead(vr::Hmd_Eye eye)
{
	vr::HmdMatrix34_t eye2head = g_VRSystem->GetEyeToHeadTransform(eye);

	return ConvertSteamVRMatrixToGLMMat(eye2head);
}

static void CreateRectMesh()
{
	// positions: rectangle in XY plane centered at 0, z=0
	float hw = 0.5f, hh = 0.5f;
	struct V
	{
		float x, y, z;
		float r, g, b;
	};
	std::vector<V> verts = {
		{-hw, -hh, 0.0f, 0.8f, 0.2f, 0.2f},
		{ hw, -hh, 0.0f, 0.2f, 0.8f, 0.2f},
		{ hw, hh, 0.0f, 0.2f, 0.2f, 0.8f},
		{-hw, hh, 0.0f, 0.8f, 0.8f, 0.2f}
	};
	std::vector<GLuint> idx = {0, 1, 2, 2, 3, 0};

	glGenVertexArrays(1, &g_RectMesh.vao);
	glBindVertexArray(g_RectMesh.vao);
	glGenBuffers(1, &g_RectMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_RectMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(V), verts.data(), GL_STATIC_DRAW);
	glGenBuffers(1, &g_RectMesh.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_RectMesh.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0); // pos
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), nullptr);
	glEnableVertexAttribArray(1); // color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), reinterpret_cast<void*>(3 * sizeof(float)));
	g_RectMesh.idxCount = static_cast<int>(idx.size());
	glBindVertexArray(0);
}

static void CreateLineMesh()
{
	// simple line with two vertices, will update dynamically
	struct LV
	{
		float x, y, z;
		float r, g, b;
	};
	std::vector<LV> verts(2);

	glGenVertexArrays(1, &g_LineMesh.vao);
	glBindVertexArray(g_LineMesh.vao);
	glGenBuffers(1, &g_LineMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_LineMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(LV) * 2, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LV), nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LV), reinterpret_cast<void*>(3 * sizeof(float)));
	g_LineMesh.idxCount = 2;
	glBindVertexArray(0);
}

static void CreatePointMesh()
{
	// simple line with two vertices, will update dynamically
	struct PV
	{
		float x, y, z;
		float r, g, b;
	};
	std::vector<PV> verts(1);

	glGenVertexArrays(1, &g_PointMesh.vao);
	glBindVertexArray(g_PointMesh.vao);
	glGenBuffers(1, &g_PointMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_PointMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(PV), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PV), nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PV), reinterpret_cast<void*>(3 * sizeof(float)));
	g_PointMesh.idxCount = 1;
	glBindVertexArray(0);
}

// Draw helpers
static void DrawMesh(const Mesh& m, const glm::mat4& mvp)
{
	glUseProgram(g_Prog);
	GLint loc = glGetUniformLocation(g_Prog, "u_mvp");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));
	glBindVertexArray(m.vao);
	glDrawElements(GL_TRIANGLES, m.idxCount, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

static void DrawLines(const Mesh& m, const glm::mat4& mvp)
{
	glUseProgram(g_Prog);
	GLint loc = glGetUniformLocation(g_Prog, "u_mvp");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));
	glBindVertexArray(m.vao);
	glDrawArrays(GL_LINES, 0, m.idxCount);
	glBindVertexArray(0);
}

static void DrawPoints(const Mesh& m, const glm::mat4& mvp)
{
	glUseProgram(g_Prog);
	GLint loc = glGetUniformLocation(g_Prog, "u_mvp");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));
	glBindVertexArray(m.vao);
	glPointSize(8.0f);
	glDrawArrays(GL_POINTS, 0, m.idxCount);
	glBindVertexArray(0);
}

// Update dynamic line buffer (controller ray)
static void UpdateLineBuffer(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color)
{
	struct LV
	{
		float x, y, z;
		float r, g, b;
	};
	LV d[2];

	d[0] = {a.x, a.y, a.z, color.r, color.g, color.b};
	d[1] = {b.x, b.y, b.z, color.r, color.g, color.b};
	glBindBuffer(GL_ARRAY_BUFFER, g_LineMesh.vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Update dynamic point buffer (controller plane intersection)
static void UpdatePointBuffer(const glm::vec3& a, const glm::vec3& color)
{
	struct PV
	{
		float x, y, z;
		float r, g, b;
	};
	PV d[1];

	d[0] = {a.x, a.y, a.z, color.r, color.g, color.b};
	glBindBuffer(GL_ARRAY_BUFFER, g_PointMesh.vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Ray-plane intersection: plane z=0 in rectangle local space (rect centered at origin, size +/-0.5)
static bool RayIntersectsRectangle(const glm::vec3& rayOrig, const glm::vec3& rayDir, glm::vec3& outHitLocal)
{
	// rectangle in local coordinates lies on plane z=0
	if (fabsf(rayDir.z) < 1e-6f)
	{
		return false;
	}
	float t = -rayOrig.z / rayDir.z;

	if (t < 0)
	{
		return false;
	}
	glm::vec3 p = rayOrig + t * rayDir;

	if ((p.x < -0.5f) || (p.x > 0.5f) || (p.y < -0.5f) || (p.y > 0.5f))
	{
		return false;
	}
	outHitLocal = p;
	return true;
}

// Get device poses
static void GetPoses(std::vector<vr::TrackedDevicePose_t>& poses) __attribute__((unused));
static void GetPoses(std::vector<vr::TrackedDevicePose_t>& poses)
{
	poses.resize(vr::k_unMaxTrackedDeviceCount);
	vr::VRCompositor()->WaitGetPoses(poses.data(), static_cast<uint32_t>(poses.size()), nullptr, 0);
}

// Main
int main()
{
	// GLFW init
	if (!glfwInit())
	{
		std::cerr << "GLFW init failed\n";
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	g_Window = glfwCreateWindow(1280, 720, "OpenVR Example", nullptr, nullptr);

	if (!g_Window)
	{
		std::cerr << "Failed to create window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(g_Window);
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		std::cerr << "GLEW init failed\n";
		return -1;
	}
	glEnable(GL_DEPTH_TEST);

	// Initialize OpenVR
	vr::EVRInitError eError = vr::VRInitError_None;
	g_VRSystem = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		PrintVRError("VR_Init failed", eError);
		return -1;
	}

	// Ensure compositor present
	if (!vr::VRCompositor())
	{
		std::cerr << "Compositor init failed\n";
		// try to init compositor explicitly
		g_VRCompositor = vr::VRCompositor();

		if (!g_VRCompositor)
		{
			std::cerr << "Failed to get IVRCompositor\n";
			vr::VR_Shutdown();
			return -1;
		}
	}
	else
	{
		g_VRCompositor = vr::VRCompositor();
	}

	// Get recommended render target size
	uint32_t renderWidth = 0, renderHeight = 0;
	g_VRSystem->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
	CreateFrameBuffer(renderWidth, renderHeight, leftEyeDesc);
	CreateFrameBuffer(renderWidth, renderHeight, rightEyeDesc);

	// create simple shader & geometry
	g_Prog = CreateSimpleProg();
	CreateRectMesh();
	CreateLineMesh();
	CreatePointMesh();

	// Create screen quad program if needed (skipped, simple blit via textured quad not implemented)
	// We'll blit the eye textures by rendering textured quads in the window (simpler path omitted to keep code concise)

	// glm::mat4 projLeft = GetHMDProjection(vr::Eye_Left, 0.1f, 100.0f);
	// glm::mat4 projRight = GetHMDProjection(vr::Eye_Right, 0.1f, 100.0f);
	// glm::mat4 eyeLeft = GetHMDEyeToHead(vr::Eye_Left);
	// glm::mat4 eyeRight = GetHMDEyeToHead(vr::Eye_Right);

	// main loop
	while (!glfwWindowShouldClose(g_Window) && g_Running)
	{
		glfwPollEvents();

		// handle simple close
		if (glfwGetKey(g_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			break;
		}

		// Get poses (WaitGetPoses also updates compositor)
		std::vector<vr::TrackedDevicePose_t> poses(vr::k_unMaxTrackedDeviceCount);
		vr::VRCompositor()->WaitGetPoses(poses.data(), static_cast<uint32_t>(poses.size()), nullptr, 0);

		// Get HMD pose
		glm::mat4 hmdPose = glm::mat4(1.0f);

		if (poses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
		{
			hmdPose = ConvertSteamVRMatrixToGLMMat(poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
		}

		// For each eye: render scene to texture
		for (int eye = 0; eye < 2; ++eye)
		{
			FramebufferDesc& fb = (eye == 0) ? leftEyeDesc : rightEyeDesc;
			glBindFramebuffer(GL_FRAMEBUFFER, fb.m_nRenderFramebufferId);
			glViewport(0, 0, fb.m_nRenderWidth, fb.m_nRenderHeight);
			glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// compute view/proj
			vr::Hmd_Eye e = (eye == 0) ? vr::Eye_Left : vr::Eye_Right;
			glm::mat4 proj = GetHMDProjection(e, 0.1f, 100.0f);
			glm::mat4 eyeToHead = GetHMDEyeToHead(e);
			glm::mat4 view = glm::inverse(hmdPose * eyeToHead);

			// draw rectangle at position in front of HMD at (0,0,-2), rotated slightly
			glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
			model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 mvp = proj * view * model;
			DrawMesh(g_RectMesh, mvp);

			// For each controller: render simple ray and do intersection with rectangle
			for (vr::TrackedDeviceIndex_t dev = 0; dev < vr::k_unMaxTrackedDeviceCount; ++dev)
			{
				if (!poses[dev].bPoseIsValid)
				{
					continue;
				}
				vr::ETrackedDeviceClass devClass = g_VRSystem->GetTrackedDeviceClass(dev);

				if ((devClass != vr::TrackedDeviceClass_Controller) && (devClass != vr::TrackedDeviceClass_GenericTracker))
				{
					continue;
				}

				glm::mat4 devPose = ConvertSteamVRMatrixToGLMMat(poses[dev].mDeviceToAbsoluteTracking);
				// controller ray: origin = device position, direction = forward -Z in device space transformed to world
				glm::vec4 origin4 = devPose * glm::vec4(0, 0, 0, 1);
				glm::vec4 forward4 = devPose * glm::vec4(0, 0, -1, 0);
				glm::vec3 origin = glm::vec3(origin4);
				glm::vec3 forward = glm::normalize(glm::vec3(forward4));

				// Transform ray into rectangle local space:
				glm::mat4 modelInv = glm::inverse(model);
				glm::vec3 rayOrigLocal = glm::vec3(modelInv * glm::vec4(origin, 1.0f));
				glm::vec3 rayDirLocal = glm::normalize(glm::vec3(modelInv * glm::vec4(origin + forward, 1.0f) - glm::vec4(rayOrigLocal, 1.0f)));

				glm::vec3 hitLocal;
				bool hit = RayIntersectsRectangle(rayOrigLocal, rayDirLocal, hitLocal);

				// draw ray (in world space)
				UpdateLineBuffer(origin, origin + forward * 5.0f, glm::vec3(0.1f, 0.9f, 0.1f));
				glm::mat4 mvpLine = proj * view * glm::mat4(1.0f);
				DrawLines(g_LineMesh, mvpLine);

				// draw point on panel
				if (hit)
				{
					UpdatePointBuffer(hitLocal, glm::vec3(0.2f, 1.0f, 0.2f));
					DrawPoints(g_PointMesh, mvp);
				}

				// handle controller input: check trigger press
				vr::VRControllerState_t state;

				if (g_VRSystem->GetControllerState(dev, &state, sizeof(state)))
				{
					// typical trigger bit: analog in 'rAxis' or touch; we check trigger button press bit
					bool triggerPressed = (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) != 0;

					if (triggerPressed && hit)
					{
						// hitLocal in rectangle local coords -> map to texture or 0..1 coords
						float u = (hitLocal.x + 0.5f);
						float v = (hitLocal.y + 0.5f);
						std::cout << "Controller " << dev << " clicked rectangle at local coords (u,v)=(" << u << "," << v << ")\n";
					}
				}
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		} // eyes

		// Submit textures to compositor
		vr::Texture_t leftEyeTexture = {reinterpret_cast<void*>(static_cast<uintptr_t>(leftEyeDesc.m_nRenderTextureId)), vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
		vr::Texture_t rightEyeTexture = {reinterpret_cast<void*>(static_cast<uintptr_t>(rightEyeDesc.m_nRenderTextureId)), vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
		vr::EVRCompositorError compErr;
		compErr = vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		if (compErr != 0)
		{

		}
		compErr = vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
		if (compErr != 0)
		{

		}

		// blit left eye RT to GLFW window for debug
		int w, h;
		glfwGetFramebufferSize(g_Window, &w, &h);
		glViewport(0, 0, w, h);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, leftEyeDesc.m_nRenderWidth, leftEyeDesc.m_nRenderHeight, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

		glfwSwapBuffers(g_Window);

		// Let compositor run
		vr::VRCompositor()->PostPresentHandoff();
	}

	// Cleanup
	vr::VR_Shutdown();
	glfwTerminate();
	return 0;
}
