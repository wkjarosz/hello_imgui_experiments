/** \file SampleViewer.h
    \author Wojciech Jarosz
*/

#pragma once

#include "common.h"
#include "linalg.h"
using namespace linalg::aliases;

// define extra conversion here before including imgui, don't do it in the imconfig.h
#define IM_VEC2_CLASS_EXTRA                                                                                            \
    constexpr ImVec2(const float2 &f) : x(f.x), y(f.y)                                                                 \
    {                                                                                                                  \
    }                                                                                                                  \
    operator float2() const                                                                                            \
    {                                                                                                                  \
        return float2(x, y);                                                                                           \
    }                                                                                                                  \
    constexpr ImVec2(const int2 &i) : x(i.x), y(i.y)                                                                   \
    {                                                                                                                  \
    }                                                                                                                  \
    operator int2() const                                                                                              \
    {                                                                                                                  \
        return int2((int)x, (int)y);                                                                                   \
    }

#define IM_VEC4_CLASS_EXTRA                                                                                            \
    constexpr ImVec4(const float4 &f) : x(f.x), y(f.y), z(f.z), w(f.w)                                                 \
    {                                                                                                                  \
    }                                                                                                                  \
    operator float4() const                                                                                            \
    {                                                                                                                  \
        return float4(x, y, z, w);                                                                                     \
    }

#include "arcball.h"
#include "hello_imgui/hello_imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "renderpass.h"
#include "shader.h"
#include <map>
#include <string>
#include <vector>

using std::map;
using std::ofstream;
using std::string;
using std::vector;

class SampleViewer
{
public:
    SampleViewer();
    virtual ~SampleViewer();

    void draw_background();
    void run();

private:
    RenderPass *m_render_pass = nullptr;
    Shader     *m_shader      = nullptr;

    map<int, ImFont *> m_regular, m_bold; // regular and bold fonts at various sizes

    float4                   m_bg_color = {0.0f, 0.0f, 0.0f, 1.f};
    HelloImGui::RunnerParams m_params;
};
