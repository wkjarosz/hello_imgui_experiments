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
#include <string_view>
#include <vector>

using std::map;
using std::ofstream;
using std::string;
using std::string_view;
using std::vector;

enum TextAlign : int
{
    // Horizontal align
    TextAlign_LEFT   = 1 << 0, // Default, align text horizontally to left.
    TextAlign_CENTER = 1 << 1, // Align text horizontally to center.
    TextAlign_RIGHT  = 1 << 2, // Align text horizontally to right.
    // Vertical align
    TextAlign_TOP    = 1 << 3, // Align text vertically to top.
    TextAlign_MIDDLE = 1 << 4, // Align text vertically to middle.
    TextAlign_BOTTOM = 1 << 5, // Align text vertically to bottom.
};

using PixelData = std::unique_ptr<float4[], void (*)(void *)>;

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
    float2 screen_position_at_pixel(float2 pixel) const;
    /**
     * Modifies the internal state of the image viewer widget so that the provided
     * position on the widget has the specified image pixel coordinate. Also clamps the values of offset
     * to the sides of the widget.
     */
    void set_pixel_at_position(float2 position, float2 pixel);

    float  pixel_ratio() const;
    int2   size() const;
    float2 size_f() const;
    float2 center_offset() const;
    float2 scaled_image_size_f() const;
    float2 image_position() const;
    float2 image_scale() const;
    /// Centers the image without affecting the scaling factor.
    void center();
    /// Centers and scales the image so that it fits inside the widget.
    void fit();
    /**
     * Changes the scale factor by the provided amount modified by the zoom sensitivity member variable.
     * The scaling occurs such that the image pixel coordinate under the focused position remains in
     * the same screen position before and after the scaling.
     */
    void zoom_by(float amount, float2 focusPosition);
    /// Zoom in to the next power of two
    void zoom_in();
    /// Zoom out to the previous power of two
    void  zoom_out();
    float zoom_level() const;
    void  set_zoom_level(float l);

    void   draw_text(float2 pos, const std::string &text, const float4 &col, ImFont *font = nullptr,
                     int align = TextAlign_RIGHT | TextAlign_BOTTOM) const;
    void   draw_pixel_info() const;
    void   draw_pixel_grid() const;
    void   draw_contents() const;
    void   draw_gui();
    void   draw_image_border() const;
    void   draw_file_window();
    void   process_hotkeys();
    float4 image_pixel(int2 p) const
    {
        return (m_image_pixels && m_image) ? m_image_pixels.get()[p.x + p.y * m_image->size().x] : float4{0.f};
    }

    RenderPass              *m_render_pass = nullptr;
    Shader                  *m_shader      = nullptr;
    std::unique_ptr<Texture> m_image = nullptr, m_null_image = nullptr, m_dither_tex = nullptr;
    PixelData                m_image_pixels;
    string                   m_filename;

    float m_exposure = 0.f, m_gamma = 2.2f;
    bool  m_sRGB = false, m_clamp_to_LDR = false, m_dither = true, m_draw_grid = true, m_draw_pixel_info = true;

    // Image display parameters.
    float      m_zoom_sensitivity = 1.0717734625f;
    float      m_zoom             = 1.f;                      ///< The scale/zoom of the image
    float2     m_offset           = {0.f, 0.f};               ///< The panning offset of the
    EChannel   m_channel          = EChannel::RGB;            ///< Which channel to display
    EBlendMode m_blend_mode       = EBlendMode::NORMAL_BLEND; ///< How to blend the current and reference images
    EBGMode    m_bg_mode          = EBGMode::BG_DARK_CHECKER; ///< How the background around the image should be
    // rendered
    float4 m_bg_color{0.3, 0.3, 0.3, 1.0}; ///< The background color if m_bg_mode == BG_CUSTOM_COLOR

    HelloImGui::RunnerParams m_params;

    vector<string> m_recent_files;

    // sans and mono fonts in both regular and bold weights at various sizes
    map<int, ImFont *> m_sans_regular, m_sans_bold, m_mono_regular, m_mono_bold;
};
