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
#include "texture.h"
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
    /// Calculates the image pixel coordinates of the given position on the widget.
    float2 pixel_at_position(float2 position) const;
    /// Calculates the position inside the widget for the given image pixel coordinate.
    float2 position_at_pixel(float2 pixel) const;
    /// Calculates the position inside the screen for the given image pixel coordinate.
    // float2 screen_position_at_pixel(float2 pixel) const;
    /**
     * Modifies the internal state of the image viewer widget so that the provided
     * position on the widget has the specified image pixel coordinate. Also clamps the values of offset
     * to the sides of the widget.
     */
    void   set_pixel_at_position(float2 position, float2 pixel);
    float2 size_f() const;
    float2 center_offset() const;
    float2 scaled_image_size_f() const;
    void   image_position_and_scale(float2 &position, float2 &scale);
    /**
     * Changes the scale factor by the provided amount modified by the zoom sensitivity member variable.
     * The scaling occurs such that the image pixel coordinate under the focused position remains in
     * the same screen position before and after the scaling.
     */
    void zoom_by(float amount, float2 focusPosition);

    RenderPass *m_render_pass = nullptr;
    Shader     *m_shader      = nullptr;
    Texture    *m_image = nullptr, *m_null_image = nullptr, *m_dither_tex = nullptr;
    int2        m_image_size{0, 0};

    float m_exposure = 0.f, m_gamma = 2.2f;
    bool  m_sRGB = false, m_clamp_to_LDR = false, m_dither = true;
    //, m_draw_grid = true, m_draw_pixel_info = true;

    // Image display parameters.
    float  m_zoom_sensitivity = 1.0717734625f;
    float  m_zoom             = 1.f;        ///< The scale/zoom of the image
    float2 m_offset           = {0.f, 0.f}; ///< The panning offset of the
    // EChannel       m_channel    = EChannel::RGB;             ///< Which channel to display
    // EBlendMode     m_blend_mode = EBlendMode::NORMAL_BLEND;  ///< How to blend the current and reference images
    // EBGMode        m_bg_mode    = EBGMode::BG_LIGHT_CHECKER; ///< How the background around the image should be
    // rendered
    float4 m_bg_color{0.f, 0.f, 0.f, 1.f}; ///< The background color if m_bg_mode == BG_CUSTOM_COLOR

    HelloImGui::RunnerParams m_params;

    map<int, ImFont *> m_regular, m_bold; // regular and bold fonts at various sizes
};
