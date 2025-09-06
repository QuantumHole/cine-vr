// SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <openvr.h>

static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 720;

// Simple linear algebra helpers
struct Vec3 { float x,y,z; };
struct Mat4 { float m[16]; };

inline Vec3 operator+(const Vec3&a,const Vec3&b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(const Vec3&a,const Vec3&b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator*(const Vec3&a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float dot(const Vec3&a,const Vec3&b){return a.x*b.x + a.y*b.y + a.z*b.z;}
inline Vec3 cross(const Vec3&a,const Vec3&b){return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};}
inline float length(const Vec3&a){return std::sqrt(dot(a,a));}
inline Vec3 normalize(const Vec3&a){float l=length(a); if(l<=0) return a; return a*(1.0f/l);}

// Convert OpenVR 3x4 to Mat4 (column-major)
static Mat4 SteamMatrixToMat4(const vr::HmdMatrix34_t &mat)
{
    Mat4 r;
    r.m[0] = mat.m[0][0]; r.m[1] = mat.m[1][0]; r.m[2] = mat.m[2][0]; r.m[3] = 0.0f;
    r.m[4] = mat.m[0][1]; r.m[5] = mat.m[1][1]; r.m[6] = mat.m[2][1]; r.m[7] = 0.0f;
    r.m[8] = mat.m[0][2]; r.m[9] = mat.m[1][2]; r.m[10]= mat.m[2][2]; r.m[11]= 0.0f;
    r.m[12]= mat.m[0][3]; r.m[13]= mat.m[1][3]; r.m[14]= mat.m[2][3]; r.m[15]= 1.0f;
    return r;
}

static Vec3 transformPoint(const Mat4 &m, const Vec3 &p){
    return {
        m.m[0]*p.x + m.m[4]*p.y + m.m[8]*p.z + m.m[12],
        m.m[1]*p.x + m.m[5]*p.y + m.m[9]*p.z + m.m[13],
        m.m[2]*p.x + m.m[6]*p.y + m.m[10]*p.z + m.m[14]
    };
}

static Vec3 transformDirection(const Mat4 &m, const Vec3 &d){
    return {
        m.m[0]*d.x + m.m[4]*d.y + m.m[8]*d.z,
        m.m[1]*d.x + m.m[5]*d.y + m.m[9]*d.z,
        m.m[2]*d.x + m.m[6]*d.y + m.m[10]*d.z
    };
}

// Shader helpers
static GLuint CompileShader(const char*src, GLenum type){
    GLuint s = glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ char buf[1024]; glGetShaderInfoLog(s,1024,nullptr,buf); std::cerr<<"Shader compile error: "<<buf<<"\n"; }
    return s;
}
static GLuint CreateProgram(const char*vs,const char*fs){
    GLuint P = glCreateProgram();
    GLuint a = CompileShader(vs,GL_VERTEX_SHADER);
    GLuint b = CompileShader(fs,GL_FRAGMENT_SHADER);
    glAttachShader(P,a); glAttachShader(P,b);
    glLinkProgram(P);
    GLint ok; glGetProgramiv(P,GL_LINK_STATUS,&ok);
    if(!ok){ char buf[1024]; glGetProgramInfoLog(P,1024,nullptr,buf); std::cerr<<"Link error: "<<buf<<"\n"; }
    glDeleteShader(a); glDeleteShader(b);
    return P;
}

const char* vs_src = R"GLSL(
#version 330 core
layout(location=0) in vec3 pos;
layout(location=1) in vec3 color;
uniform mat4 uMVP;
out vec3 vColor;
void main(){ vColor=color; gl_Position=uMVP*vec4(pos,1.0); }
)GLSL";

const char* fs_src = R"GLSL(
#version 330 core
in vec3 vColor;
out vec4 outColor;
void main(){ outColor=vec4(vColor,1.0); }
)GLSL";

struct Rect {
    Vec3 center;
    float width, height;
    Vec3 right, up;
};

static bool RayIntersectRect(const Vec3& rayOrig, const Vec3& rayDir, const Rect& R, Vec3 &hitPoint, float &u, float &v) {
    Vec3 normal = normalize(cross(R.right, R.up));
    float denom = dot(normal, rayDir);
    if (fabsf(denom) < 1e-6f) return false;
    float t = dot(normal, R.center - rayOrig) / denom;
    if (t < 0) return false;
    hitPoint = rayOrig + rayDir * t;
    Vec3 local = hitPoint - R.center;
    Vec3 rnorm = normalize(R.right), unorm = normalize(R.up);
    float projU = dot(local, rnorm);
    float projV = dot(local, unorm);
    if (fabsf(projU) <= R.width*0.5f && fabsf(projV) <= R.height*0.5f) {
        u = (projU / (R.width*0.5f) + 1.0f) * 0.5f;
        v = (projV / (R.height*0.5f) + 1.0f) * 0.5f;
        return true;
    }
    return false;
}

// Matrix multiply (column-major)
static Mat4 mat4Mul(const Mat4&a,const Mat4&b){
    Mat4 r;
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            float s=0;
            for(int k=0;k<4;k++) s += a.m[k*4 + i] * b.m[j*4 + k];
            r.m[j*4 + i] = s;
        }
    }
    return r;
}

static Mat4 perspective_from_fovy(float fovy, float aspect, float zn, float zf)__attribute__((unused));
static Mat4 perspective_from_fovy(float fovy, float aspect, float zn, float zf){
    float f = 1.0f / tanf(fovy*0.5f);
    Mat4 m = {};
    m.m[0] = f/aspect; m.m[5] = f; m.m[10] = (zf+zn)/(zn-zf); m.m[11] = -1.0f;
    m.m[14] = (2*zf*zn)/(zn-zf); m.m[15] = 0.0f;
    return m;
}

int main(){
    if (!glfwInit()){ std::cerr<<"Failed to init GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Cine-VR", nullptr, nullptr);
    if (!window){ std::cerr<<"GLFW window failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK){ std::cerr<<"Failed to init GLEW\n"; return -1; }
    glEnable(GL_DEPTH_TEST);

    // OpenVR init
    vr::EVRInitError eError = vr::VRInitError_None;
    vr::IVRSystem *vrsys = vr::VR_Init(&eError, vr::VRApplication_Scene);
    if (eError != vr::VRInitError_None) {
        std::cerr << "Failed to init OpenVR: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError) << "\n";
        return -1;
    }

    // Get recommended render target size
    uint32_t rtWidth = 0, rtHeight = 0;
    vrsys->GetRecommendedRenderTargetSize(&rtWidth, &rtHeight);
    std::cout << "Recommended RT size: " << rtWidth << "x" << rtHeight << "\n";

    struct EyeRenderTarget { GLuint framebuffer; GLuint texture; GLuint depthbuffer; };
    EyeRenderTarget eyeRT[2];

    auto CreateEyeRT = [&](EyeRenderTarget &rt, uint32_t w, uint32_t h){
        glGenFramebuffers(1, &rt.framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, rt.framebuffer);

        glGenTextures(1, &rt.texture);
        glBindTexture(GL_TEXTURE_2D, rt.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt.texture, 0);

        glGenRenderbuffers(1, &rt.depthbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, rt.depthbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt.depthbuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Incomplete framebuffer\n";
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };

    CreateEyeRT(eyeRT[0], rtWidth, rtHeight);
    CreateEyeRT(eyeRT[1], rtWidth, rtHeight);

    // Shaders and geometry
    GLuint prog = CreateProgram(vs_src, fs_src);
    GLint uMVP = glGetUniformLocation(prog, "uMVP");

    // Rectangle geometry (local space centered)
    Rect rect;
    rect.center = {0.0f, 1.2f, -1.5f};
    rect.width = 0.8f;
    rect.height = 0.5f;
    rect.right = {1.0f, 0.0f, 0.0f};
    rect.up = {0.0f, 1.0f, 0.0f};
    float hw = rect.width*0.5f, hh = rect.height*0.5f;
    GLfloat rectVerts[] = {
        -hw, -hh, 0.0f,  0.7f,0.7f,0.9f,
         hw, -hh, 0.0f,  0.7f,0.7f,0.9f,
         hw,  hh, 0.0f,  0.9f,0.9f,0.7f,
        -hw,  hh, 0.0f,  0.9f,0.9f,0.7f
    };
    GLuint rectIdx[] = {0,1,2, 2,3,0};
    GLuint rectVao, rectVbo, rectEbo;
    glGenVertexArrays(1,&rectVao); glBindVertexArray(rectVao);
    glGenBuffers(1,&rectVbo); glBindBuffer(GL_ARRAY_BUFFER,rectVbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(rectVerts),rectVerts,GL_STATIC_DRAW);
    glGenBuffers(1,&rectEbo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,rectEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(rectIdx),rectIdx,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<void*>(3*sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Temporary VBOs for lines/points will be created on the fly (simple)

    // Poses
    vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];

    // Controller trigger previous state for rising-edge detection
    bool prevTriggerPressed[vr::k_unMaxTrackedDeviceCount] = {0};

    // Helpers: projection and eye transforms from OpenVR
    auto GetEyeProjection = [&](vr::Hmd_Eye eye, float nearClip, float farClip)->Mat4{
        vr::HmdMatrix44_t proj = vrsys->GetProjectionMatrix(eye, nearClip, farClip);
        Mat4 pm;
        for(int r=0;r<4;r++) for(int c=0;c<4;c++) pm.m[c*4 + r] = proj.m[r][c];
        return pm;
    };
    auto GetEyeToHead = [&](vr::Hmd_Eye eye)->Mat4{
        vr::HmdMatrix34_t eyeToHead = vrsys->GetEyeToHeadTransform(eye);
        return SteamMatrixToMat4(eyeToHead);
    };

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Let compositor update poses (recommended)
        vr::VRCompositor()->WaitGetPoses(poses, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

        // HMD pose (world transform)
        Mat4 hmdPose;
        bool haveHmdPose = poses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid;
        if (haveHmdPose) hmdPose = SteamMatrixToMat4(poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
        else { for(int i=0;i<16;i++) hmdPose.m[i] = (i%5==0)?1.0f:0.0f; }

        // For each eye
        for (int eye = 0; eye < 2; ++eye) {
            vr::Hmd_Eye e = eye==0 ? vr::Eye_Left : vr::Eye_Right;

            glBindFramebuffer(GL_FRAMEBUFFER, eyeRT[eye].framebuffer);
            glViewport(0,0, rtWidth, rtHeight);
            glClearColor(0.1f,0.12f,0.14f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Compute view-proj for eye
            Mat4 eyeToHead = GetEyeToHead(e);
            Mat4 headEye = mat4Mul(hmdPose, eyeToHead); // world transform of eye
            // invert rigid transform
            Mat4 view = {};
            for(int i=0;i<3;i++) for(int j=0;j<3;j++) view.m[j*4 + i] = headEye.m[i*4 + j];
            Vec3 tEye = { headEye.m[12], headEye.m[13], headEye.m[14] };
            Vec3 ntEye = transformDirection(view, {-tEye.x,-tEye.y,-tEye.z});
            view.m[12]=ntEye.x; view.m[13]=ntEye.y; view.m[14]=ntEye.z; view.m[15]=1.0f;

            Mat4 projEye = GetEyeProjection(e, 0.01f, 100.0f);
            Mat4 vpEye = mat4Mul(projEye, view);

            // Render rectangle
            glUseProgram(prog);
            // model matrix from rect basis + translation
            Mat4 model = {};
            Vec3 r = normalize(rect.right), u = normalize(rect.up), n = normalize(cross(r,u));
            model.m[0]=r.x; model.m[1]=r.y; model.m[2]=r.z; model.m[3]=0;
            model.m[4]=u.x; model.m[5]=u.y; model.m[6]=u.z; model.m[7]=0;
            model.m[8]=n.x; model.m[9]=n.y; model.m[10]=n.z; model.m[11]=0;
            model.m[12]=rect.center.x; model.m[13]=rect.center.y; model.m[14]=rect.center.z; model.m[15]=1.0f;
            Mat4 mvpRect = mat4Mul(vpEye, model);
            glUniformMatrix4fv(uMVP,1,GL_FALSE,mvpRect.m);
            glBindVertexArray(rectVao);
            glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);
            glBindVertexArray(0);

            // Render controllers and do picking (use world-space vpEye)
            for (vr::TrackedDeviceIndex_t dev = 0; dev < vr::k_unMaxTrackedDeviceCount; ++dev) {
                if (!poses[dev].bDeviceIsConnected || !poses[dev].bPoseIsValid) continue;
                vr::ETrackedDeviceClass devClass = vrsys->GetTrackedDeviceClass(dev);
                if (devClass != vr::TrackedDeviceClass_Controller && devClass != vr::TrackedDeviceClass_GenericTracker && devClass != vr::TrackedDeviceClass_HMD) continue;

                Mat4 devMat = SteamMatrixToMat4(poses[dev].mDeviceToAbsoluteTracking);
                Vec3 devPos = transformPoint(devMat,{0,0,0});
                Vec3 forward = transformDirection(devMat, {0,0,-1});
                forward = normalize(forward);

                // Draw controller forward line
                GLfloat lineVerts[] = {
                    devPos.x, devPos.y, devPos.z,  1.0f,0.2f,0.2f,
                    devPos.x + forward.x*0.25f, devPos.y + forward.y*0.25f, devPos.z + forward.z*0.25f,  1.0f,0.8f,0.2f
                };
                GLuint tmpVao, tmpVbo;
                glGenVertexArrays(1,&tmpVao); glBindVertexArray(tmpVao);
                glGenBuffers(1,&tmpVbo); glBindBuffer(GL_ARRAY_BUFFER,tmpVbo);
                glBufferData(GL_ARRAY_BUFFER,sizeof(lineVerts),lineVerts,GL_DYNAMIC_DRAW);
                glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr); glEnableVertexAttribArray(0);
                glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<void*>(3*sizeof(float))); glEnableVertexAttribArray(1);
                Mat4 identity = {};
                for(int i=0;i<16;i++) identity.m[i] = (i%5==0)?1.0f:0.0f;
                Mat4 mvpLine = mat4Mul(vpEye, identity);
                glUniformMatrix4fv(uMVP,1,GL_FALSE,mvpLine.m);
                glBindVertexArray(tmpVao);
                glDrawArrays(GL_LINES,0,2);
                glBindVertexArray(0);
                glDeleteBuffers(1,&tmpVbo); glDeleteVertexArrays(1,&tmpVao);

                // Ray-rect intersection
                Vec3 hitPoint; float uu,vv;
                bool hit = RayIntersectRect(devPos, forward, rect, hitPoint, uu, vv);
                if (hit) {
                    GLfloat pvert[] = { hitPoint.x, hitPoint.y, hitPoint.z,  0.2f,1.0f,0.2f };
                    GLuint pVao, pVbo;
                    glGenVertexArrays(1,&pVao); glBindVertexArray(pVao);
                    glGenBuffers(1,&pVbo); glBindBuffer(GL_ARRAY_BUFFER,pVbo);
                    glBufferData(GL_ARRAY_BUFFER,sizeof(pvert),pvert,GL_DYNAMIC_DRAW);
                    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr); glEnableVertexAttribArray(0);
                    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<void*>(3*sizeof(float))); glEnableVertexAttribArray(1);
                    Mat4 mvpP = mat4Mul(vpEye, identity);
                    glUniformMatrix4fv(uMVP,1,GL_FALSE,mvpP.m);
                    glPointSize(8.0f);
                    glBindVertexArray(pVao);
                    glDrawArrays(GL_POINTS,0,1);
                    glBindVertexArray(0);
                    glDeleteBuffers(1,&pVbo); glDeleteVertexArrays(1,&pVao);
                }

                // Controller button/axis state
                vr::VRControllerState_t state;
                if (vrsys->GetControllerState(dev, &state, sizeof(state))) {
                    bool triggerPressed = false;
                    // common mapping: trigger often rAxis[1].x or rAxis[1].y; check both
                    if (state.rAxis[1].x > 0.5f || state.rAxis[1].y > 0.5f) triggerPressed = true;
                    if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) triggerPressed = true;

                    if (triggerPressed && !prevTriggerPressed[dev]) {
                        Vec3 hitPoint2; float uu2,vv2;
                        if (RayIntersectRect(devPos, forward, rect, hitPoint2, uu2, vv2)) {
                            float x_m = uu2 * rect.width;
                            float y_m = vv2 * rect.height;
                            std::cout << "Controller " << dev << " clicked rectangle at normalized (u,v)=("
                                      << uu2 << ", " << vv2 << ") -> meters (" << x_m << ", " << y_m << ")\n";
                        } else {
                            std::cout << "Controller " << dev << " clicked but missed rectangle\n";
                        }
                    }
                    prevTriggerPressed[dev] = triggerPressed;
                }
            } // end devices loop

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Submit eye texture to compositor
            vr::Texture_t tex = { reinterpret_cast<void*>(static_cast<intptr_t>(eyeRT[eye].texture)), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
            vr::EVRCompositorError compErr = vr::VRCompositor()->Submit(e, &tex);
            if (compErr != vr::VRCompositorError_None) {
                std::cerr << "VRCompositor submit error: " << compErr << "\n";
            }
        } // end eyes

        // Optional: blit left eye RT to GLFW window for debug
        int w,h; glfwGetFramebufferSize(window,&w,&h);
        glViewport(0,0,w,h);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, eyeRT[0].framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0,0,rtWidth,rtHeight, 0,0,w,h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        // Allow compositor to present
        vr::VRCompositor()->PostPresentHandoff();

        glfwSwapBuffers(window);
    }

    // Cleanup
    vr::VR_Shutdown();
    glDeleteProgram(prog);
    glDeleteBuffers(1,&rectVbo);
    glDeleteBuffers(1,&rectEbo);
    glDeleteVertexArrays(1,&rectVao);
    for(int i=0;i<2;i++){
        glDeleteFramebuffers(1,&eyeRT[i].framebuffer);
        glDeleteTextures(1,&eyeRT[i].texture);
        glDeleteRenderbuffers(1,&eyeRT[i].depthbuffer);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
