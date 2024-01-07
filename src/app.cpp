/** \file SampleViewer.cpp
    \author Wojciech Jarosz
*/

#include "app.h"

#include "hello_imgui/hello_imgui.h"
#include "hello_imgui/hello_imgui_include_opengl.h" // cross-platform way to include OpenGL headers
#include "imgui_ext.h"
#include "imgui_internal.h"

#include "opengl_check.h"

#include "dithermatrix256.h"
#include "texture.h"
#include "timer.h"

#include <cmath>
#include <fmt/core.h>
#include <fstream>
#include <random>
#include <utility>

#ifdef __EMSCRIPTEN__
#include "emscripten_browser_file.h"
#include <string_view>
using std::string_view;
#else
#include "portable-file-dialogs.h"
#endif

#ifdef HELLOIMGUI_USE_SDL_OPENGL3
#include <SDL.h>
#endif

#include "stb_image.h"

using namespace linalg::ostream_overloads;

using std::to_string;

#ifdef __EMSCRIPTEN__
EM_JS(int, screen_width, (), { return screen.width; });
EM_JS(int, screen_height, (), { return screen.height; });
EM_JS(int, window_width, (), { return window.innerWidth; });
EM_JS(int, window_height, (), { return window.innerHeight; });
#endif

static std::mt19937 g_rand(53);
static const float  MIN_ZOOM = 0.01f;
static const float  MAX_ZOOM = 512.f;

SampleViewer::SampleViewer() : m_image_pixels(nullptr, stbi_image_free)
{
    stbi_ldr_to_hdr_scale(1.0f);
    stbi_ldr_to_hdr_gamma(1.0f);

    // set up HelloImGui parameters
    m_params.appWindowParams.windowGeometry.size     = {1200, 800};
    m_params.appWindowParams.windowTitle             = "HelloGuiExperiments";
    m_params.appWindowParams.restorePreviousGeometry = false;

    // Menu bar
    m_params.imGuiWindowParams.showMenuBar            = true;
    m_params.imGuiWindowParams.showStatusBar          = true;
    m_params.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;

    // Setting this to true allows multiple viewports where you can drag windows outside out the main window in order to
    // put their content into new native windows m_params.imGuiWindowParams.enableViewports = true;
    m_params.imGuiWindowParams.enableViewports = false;
    m_params.imGuiWindowParams.menuAppTitle    = "File";

    m_params.iniFolderType = HelloImGui::IniFolderType::AppUserConfigFolder;
    m_params.iniFilename   = "HelloGuiExperiments/settings.ini";

    // A console window named "Console" will be placed in "ConsoleSpace". It uses the HelloImGui logger gui
    HelloImGui::DockableWindow consoleWindow;
    consoleWindow.label             = "Console";
    consoleWindow.dockSpaceName     = "ConsoleSpace";
    consoleWindow.isVisible         = false;
    consoleWindow.rememberIsVisible = true;
    consoleWindow.GuiFunction       = [] { HelloImGui::LogGui(); };

    // docking layouts
    {
        m_params.dockingParams.layoutName      = "Settings on left";
        m_params.dockingParams.dockableWindows = {consoleWindow};

        HelloImGui::DockingSplit splitMainConsole{"MainDockSpace", "ConsoleSpace", ImGuiDir_Down, 0.25f};

        m_params.dockingParams.dockingSplits = {
            HelloImGui::DockingSplit{"MainDockSpace", "EditorSpace", ImGuiDir_Left, 0.2f}, splitMainConsole};

        HelloImGui::DockingParams right_layout, portrait_layout, landscape_layout;

        right_layout.layoutName      = "Settings on right";
        right_layout.dockableWindows = {consoleWindow};
        right_layout.dockingSplits   = {HelloImGui::DockingSplit{"MainDockSpace", "EditorSpace", ImGuiDir_Right, 0.2f},
                                        splitMainConsole};

        consoleWindow.dockSpaceName = "EditorSpace";

        portrait_layout.layoutName      = "Mobile device (portrait orientation)";
        portrait_layout.dockableWindows = {consoleWindow};
        portrait_layout.dockingSplits = {HelloImGui::DockingSplit{"MainDockSpace", "EditorSpace", ImGuiDir_Down, 0.5f}};

        landscape_layout.layoutName      = "Mobile device (landscape orientation)";
        landscape_layout.dockableWindows = {consoleWindow};
        landscape_layout.dockingSplits   = {
            HelloImGui::DockingSplit{"MainDockSpace", "EditorSpace", ImGuiDir_Left, 0.5f}};

        m_params.alternativeDockingLayouts = {right_layout, portrait_layout, landscape_layout};

#ifdef __EMSCRIPTEN__
        HelloImGui::Log(HelloImGui::LogLevel::Info, "Screen size: %d, %d\nWindow size: %d, %d", screen_width(),
                        screen_height(), window_width(), window_height());
        if (std::min(screen_width(), screen_height()) < 500)
        {
            HelloImGui::Log(
                HelloImGui::LogLevel::Info, "Switching to %s layout",
                m_params.alternativeDockingLayouts[window_width() < window_height() ? 1 : 2].layoutName.c_str());
            std::swap(m_params.dockingParams,
                      m_params.alternativeDockingLayouts[window_width() < window_height() ? 1 : 2]);
        }
#endif
    }

    m_params.imGuiWindowParams.backgroundColor = float4{0.15f, 0.15f, 0.15f, 1.f};

    m_params.callbacks.LoadAdditionalFonts = [this]()
    {
        std::string sans_r = "fonts/Roboto/Roboto-Regular.ttf";
        std::string sans_b = "fonts/Roboto/Roboto-Bold.ttf";
        // std::string mono_r = "fonts/Roboto/RobotoMono-Regular.ttf";
        // std::string mono_b = "fonts/Roboto/RobotoMono-Bold.ttf";
        std::string mono_r = "fonts/Inconsolata-Regular.ttf";
        std::string mono_b = "fonts/Inconsolata-Bold.ttf";
        if (!HelloImGui::AssetExists(sans_r) || !HelloImGui::AssetExists(sans_b) || !HelloImGui::AssetExists(mono_r) ||
            !HelloImGui::AssetExists(mono_b))
            return;

        for (auto font_size : {14, 10, 16, 18, 30})
        {
            m_sans_regular[font_size] = HelloImGui::LoadFontTTF_WithFontAwesomeIcons(sans_r, (float)font_size);
            m_sans_bold[font_size]    = HelloImGui::LoadFontTTF_WithFontAwesomeIcons(sans_b, (float)font_size);
            m_mono_regular[font_size] = HelloImGui::LoadFontTTF(mono_r, (float)font_size);
            m_mono_bold[font_size]    = HelloImGui::LoadFontTTF(mono_b, (float)font_size);
        }
    };

    m_params.callbacks.ShowStatus = [this]()
    {
        if (m_image && m_image_pixels)
        {
            auto &io = ImGui::GetIO();

            int2 p(pixel_at_position(io.MousePos));
            ImGui::PushFont(m_mono_regular[14]);

            if (p.x > 0 && p.y > 0 && p.x < m_image->size().x && p.y < m_image->size().y)
            {
                float4 color32 = image_pixel(p);
                float4 color8  = linalg::clamp(color32 * pow(2.f, m_exposure) * 255, 0.f, 255.f);

                // ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetFontSize() * 0.15f);
                ImGui::TextUnformatted(
                    fmt::format("({:>4d},{:>4d}) = ({:>6.3f},{:>6.3f},{:>6.3f},{:>6.3f})/({:>3d},{:>3d},{:>3d},{:>3d})",
                                p.x, p.y, color32.x, color32.y, color32.z, color32.w, (int)round(color8.x),
                                (int)round(color8.y), (int)round(color8.z), (int)round(color8.w))
                        .c_str());
                // ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetFontSize() * 0.15f);
            }

            // ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetFontSize() * 0.15f);
            float  real_zoom = m_zoom * pixel_ratio();
            int    numer     = (real_zoom < 1.0f) ? 1 : (int)round(real_zoom);
            int    denom     = (real_zoom < 1.0f) ? (int)round(1.0f / real_zoom) : 1;
            auto   text      = fmt::format("{:7.2f}% ({:d}:{:d})", real_zoom * 100, numer, denom);
            float2 text_size = ImGui::CalcTextSize(text.c_str());
            ImGui::SameLine(ImGui::GetIO().DisplaySize.x - text_size.x - 16.f * ImGui::GetFontSize());
            ImGui::TextUnformatted(text.c_str());
            // ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetFontSize() * 0.15f);
            ImGui::PopFont();
        }
    };

    m_params.callbacks.SetupImGuiStyle = [this]()
    {
        try
        {
            m_render_pass = new RenderPass(false, true);
            m_render_pass->set_cull_mode(RenderPass::CullMode::Disabled);
            m_render_pass->set_depth_test(RenderPass::DepthTest::Always, false);
            m_render_pass->set_clear_color(float4(0.15f, 0.15f, 0.15f, 1.f));

            m_shader = new Shader(m_render_pass,
                                  /* An identifying name */
                                  "ImageView", Shader::from_asset("shaders/image-shader_vert"),
                                  Shader::prepend_includes(Shader::from_asset("shaders/image-shader_frag"),
                                                           {"shaders/colorspaces", "shaders/colormaps"}),
                                  Shader::BlendMode::AlphaBlend);

            const float positions[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f, -1.f, 1.f};

            m_shader->set_buffer("position", VariableType::Float32, {6, 2}, positions);
            m_render_pass->set_cull_mode(RenderPass::CullMode::Disabled);

            m_dither_tex = new Texture(Texture::PixelFormat::R, Texture::ComponentFormat::Float32, {256, 256},
                                       Texture::InterpolationMode::Nearest, Texture::InterpolationMode::Nearest,
                                       Texture::WrapMode::Repeat);
            m_dither_tex->upload((const uint8_t *)dither_matrix256);
            m_shader->set_texture("dither_texture", m_dither_tex);

            // create an empty texture so that the shader doesn't print errors
            // before we've selected a reference image
            // FIXME: at some point, find a more elegant solution for this.
            m_null_image = new Texture(Texture::PixelFormat::RGBA, Texture::ComponentFormat::Float32, {1, 1},
                                       Texture::InterpolationMode::Nearest, Texture::InterpolationMode::Nearest,
                                       Texture::WrapMode::Repeat);
            static const float4 null_tex{1.f, 1.f, 1.f, 1.f};
            m_null_image->upload((const uint8_t *)&null_tex);
            m_shader->set_texture("secondary_texture", m_null_image);
            m_shader->set_texture("primary_texture", m_null_image);

            HelloImGui::Log(HelloImGui::LogLevel::Info, "Successfully initialized graphics API!");
        }
        catch (const std::exception &e)
        {
            fmt::print(stderr, "Shader initialization failed!:\n\t{}.", e.what());
            HelloImGui::Log(HelloImGui::LogLevel::Error, "Shader initialization failed!:\n\t%s.", e.what());
        }
    };

    // m_params.callbacks.PostInit = [this]() {

    // };

    m_params.callbacks.ShowAppMenuItems = [this]()
    {
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open image..."))
        {
#ifndef __EMSCRIPTEN__
            auto result = pfd::open_file("Open image", "", {"Image files", "*.png *.hdr *.jpeg *.jpg *.bmp"}).result();
            if (!result.empty())
            {
                HelloImGui::Log(HelloImGui::LogLevel::Debug, "Loading file '%s'...", result.front().c_str());
                delete m_image;
                // m_image         = new Texture(result.front(), Texture::InterpolationMode::Trilinear,
                //                               Texture::InterpolationMode::Nearest, Texture::WrapMode::ClampToEdge);
                string filename = result.front();
                int2   image_size;
                m_image_pixels = PixelData{
                    (float4 *)stbi_loadf(filename.c_str(), &image_size.x, &image_size.y, nullptr, 4), stbi_image_free};
                if (!m_image_pixels)
                    throw std::runtime_error("Could not load texture data from file \"" + filename +
                                             "\". Reason: " + stbi_failure_reason());
                m_image = new Texture(Texture::PixelFormat::RGBA, Texture::ComponentFormat::Float32, image_size,
                                      Texture::InterpolationMode::Trilinear, Texture::InterpolationMode::Nearest,
                                      Texture::WrapMode::ClampToEdge, 1, Texture::TextureFlags::ShaderRead);
                if (m_image->pixel_format() != Texture::PixelFormat::RGBA)
                    throw std::runtime_error("Pixel format not supported by the hardware!");
                m_image->upload((const uint8_t *)m_image_pixels.get());
                m_shader->set_texture("primary_texture", m_image);
            }
#else
            auto handle_upload_file =
                [](const string &filename, const string &mime_type, string_view buffer, void *my_data = nullptr)
            {
                auto that{reinterpret_cast<SampleViewer *>(my_data)};
                HelloImGui::Log(HelloImGui::LogLevel::Debug, "Loading file '%s' of mime type '%s' ...",
                                filename.c_str(), mime_type.c_str());
                delete that->m_image;
                // that->m_image = new Texture(filename, buffer, Texture::InterpolationMode::Trilinear,
                //                             Texture::InterpolationMode::Nearest, Texture::WrapMode::ClampToEdge);
                int2 image_size;
                that->m_image_pixels =
                    PixelData{(float4 *)stbi_loadf_from_memory((const stbi_uc *)buffer.begin(), buffer.length(),
                                                               &image_size.x, &image_size.y, nullptr, 4),
                              stbi_image_free};
                if (!that->m_image_pixels)
                    throw std::runtime_error("Could not load texture data from file \"" + filename +
                                             "\". Reason: " + stbi_failure_reason());
                that->m_image = new Texture(Texture::PixelFormat::RGBA, Texture::ComponentFormat::Float32, image_size,
                                            Texture::InterpolationMode::Trilinear, Texture::InterpolationMode::Nearest,
                                            Texture::WrapMode::ClampToEdge, 1, Texture::TextureFlags::ShaderRead);
                if (that->m_image->pixel_format() != Texture::PixelFormat::RGBA)
                    throw std::runtime_error("Pixel format not supported by the hardware!");
                that->m_image->upload((const uint8_t *)that->m_image_pixels.get());
                that->m_shader->set_texture("primary_texture", that->m_image);
            };
            // open the browser's file selector, and pass the file to the upload handler
            emscripten_browser_file::upload(".png,.hdr,.jpg,.jpeg,.bmp", handle_upload_file, this);
            HelloImGui::Log(HelloImGui::LogLevel::Debug, "Requesting file from user");
#endif
            if (m_image)
                fmt::print("Loaded image of size: {},{}\n", m_image->size().x, m_image->size().y);
        }
    };

    m_params.callbacks.CustomBackground = [this]() { draw_background(); };
}

void SampleViewer::run()
{
    HelloImGui::Run(m_params);
}

SampleViewer::~SampleViewer()
{
}

void SampleViewer::center()
{
    m_offset = float2(0.f, 0.f);
}

void SampleViewer::fit()
{
    // Calculate the appropriate scaling factor.
    m_zoom = minelem(size_f() / m_image->size());
    center();
}

float SampleViewer::zoom_level() const
{
    return log(m_zoom * pixel_ratio()) / log(m_zoom_sensitivity);
}

void SampleViewer::set_zoom_level(float level)
{
    m_zoom = std::clamp(std::pow(m_zoom_sensitivity, level) / pixel_ratio(), MIN_ZOOM, MAX_ZOOM);
}

void SampleViewer::zoom_by(float amount, float2 focus_pos)
{
    if (amount == 0.f)
        return;

    auto  focused_pixel = pixel_at_position(focus_pos);
    float scale_factor  = std::pow(m_zoom_sensitivity, amount);
    m_zoom              = std::clamp(scale_factor * m_zoom, MIN_ZOOM, MAX_ZOOM);
    set_pixel_at_position(focus_pos, focused_pixel);
}

void SampleViewer::zoom_in()
{
    // keep position at center of window fixed while zooming
    auto center_pos   = float2(size_f() / 2.f);
    auto center_pixel = pixel_at_position(center_pos);

    // determine next higher power of 2 zoom level
    float level_for_sensitivity = std::ceil(log(m_zoom) / log(2.f) + 0.5f);
    float new_scale             = std::pow(2.f, level_for_sensitivity);
    m_zoom                      = std::clamp(new_scale, MIN_ZOOM, MAX_ZOOM);
    set_pixel_at_position(center_pos, center_pixel);
}

void SampleViewer::zoom_out()
{
    // keep position at center of window fixed while zooming
    auto center_pos   = float2(size_f() / 2.f);
    auto center_pixel = pixel_at_position(center_pos);

    // determine next lower power of 2 zoom level
    float level_for_sensitivity = std::floor(log(m_zoom) / log(2.f) - 0.5f);
    float new_scale             = std::pow(2.f, level_for_sensitivity);
    m_zoom                      = std::clamp(new_scale, MIN_ZOOM, MAX_ZOOM);
    set_pixel_at_position(center_pos, center_pixel);
}

float2 SampleViewer::pixel_at_position(float2 position) const
{
    auto image_pos = position - (m_offset + center_offset());
    return image_pos / m_zoom;
}

float2 SampleViewer::position_at_pixel(float2 pixel) const
{
    return m_zoom * pixel + (m_offset + center_offset());
}

float2 SampleViewer::screen_position_at_pixel(float2 pixel) const
{
    // return position_at_pixel(pixel) + position_f();
    return position_at_pixel(pixel) + float2{0.f};
}

void SampleViewer::set_pixel_at_position(float2 position, float2 pixel)
{
    // Calculate where the new offset must be in order to satisfy the image position equation.
    // Round the floating point values to balance out the floating point to integer conversions.
    m_offset = position - (pixel * m_zoom);

    // Clamp offset so that the image remains near the screen.
    m_offset = max(min(m_offset, size_f()), -scaled_image_size_f());

    m_offset -= center_offset();
}

float2 SampleViewer::scaled_image_size_f() const
{
    return m_zoom * (m_image ? m_image->size() : int2{0.f});
}

float SampleViewer::pixel_ratio() const
{
    return ImGui::GetIO().DisplayFramebufferScale.x;
}

int2 SampleViewer::size() const
{
    return int2{size_f()};
}

float2 SampleViewer::size_f() const
{
    return float2{ImGui::GetIO().DisplaySize};
}

float2 SampleViewer::center_offset() const
{
    return (size_f() - scaled_image_size_f()) / 2.f;
}

float2 SampleViewer::image_position() const
{
    return (m_offset + center_offset()) / size_f();
}

float2 SampleViewer::image_scale() const
{
    return scaled_image_size_f() / size_f();
}

void SampleViewer::draw_text(float2 pos, const string &text, const float4 &color, ImFont *font, int align) const
{
    ImGui::PushFont(font);
    float2 size = ImGui::CalcTextSize(text.c_str());

    // if (align & TextAlign_LEFT)
    //     pos.x = pos.x;
    if (align & TextAlign_CENTER)
        pos.x -= 0.5f * size.x;
    else if (align & TextAlign_RIGHT)
        pos.x -= size.x;

    // if (align & TextAlign_TOP)
    //     pos.y = pos.y;
    if (align & TextAlign_MIDDLE)
        pos.y -= 0.5f * size.y;
    else if (align & TextAlign_BOTTOM)
        pos.y -= size.y;

    ImGui::GetBackgroundDrawList()->AddText(pos, ImColor(color), text.c_str());
    ImGui::PopFont();
}

void SampleViewer::draw_pixel_grid() const
{
    if (!m_image)
        return;

    static const bool m_draw_grid      = true;
    static const int  m_grid_threshold = 10;

    if (!m_draw_grid || (m_grid_threshold == -1) || (m_zoom <= m_grid_threshold))
        return;

    float factor = std::clamp((m_zoom - m_grid_threshold) / (2 * m_grid_threshold), 0.f, 1.f);
    float alpha  = lerp(0.0f, 0.2f, smoothstep(0.0f, 1.0f, factor));

    if (alpha > 0.0f)
    {
        ImColor col(1.0f, 1.0f, 1.0f, alpha);

        float2 xy0  = screen_position_at_pixel(float2(0.0f));
        int    minJ = std::max(0, int(-xy0.y / m_zoom));
        int    maxJ = std::min(m_image->size().y - 1, int(ceil((size().y - xy0.y) / m_zoom)));
        int    minI = std::max(0, int(-xy0.x / m_zoom));
        int    maxI = std::min(m_image->size().x - 1, int(ceil((size().x - xy0.x) / m_zoom)));

        // draw vertical lines
        for (int i = minI; i <= maxI; ++i)
            ImGui::GetBackgroundDrawList()->AddLine(screen_position_at_pixel(float2(i, minJ)),
                                                    screen_position_at_pixel(float2(i, maxJ)), col, 2.f);

        // draw horizontal lines
        for (int j = minJ; j <= maxJ; ++j)
            ImGui::GetBackgroundDrawList()->AddLine(screen_position_at_pixel(float2(minI, j)),
                                                    screen_position_at_pixel(float2(maxI, j)), col, 2.f);
    }
}

void SampleViewer::draw_pixel_info() const
{
    if (!m_image)
        return;

    constexpr int align     = TextAlign_CENTER | TextAlign_MIDDLE;
    constexpr int font_size = 30;
    auto          font      = m_mono_bold.at(font_size);

    static const bool m_draw_pixel_info = true;

    ImGui::PushFont(font);
    static float        line_height     = ImGui::CalcTextSize("").y;
    static const float2 RGBA_threshold2 = float2{ImGui::CalcTextSize("R: 1.000").x, 4.f * line_height};
    static const float2 XY_threshold2   = RGBA_threshold2 + float2{0.f, 2.f * line_height};
    static const float  RGBA_threshold  = maxelem(RGBA_threshold2);
    static const float  XY_threshold    = maxelem(XY_threshold2);
    ImGui::PopFont();

    if (!m_draw_pixel_info || m_zoom <= RGBA_threshold)
        return;

    // fade value for the R,G,B,A values shown at sufficient zoom
    float factor = std::clamp((m_zoom - RGBA_threshold) / (1.25f * RGBA_threshold), 0.f, 1.f);
    float alpha  = lerp(0.0f, 1.0f, smoothstep(0.0f, 1.0f, factor));

    // fade value for the (x,y) coordinates shown at further zoom
    float factor2 = std::clamp((m_zoom - XY_threshold) / (1.25f * XY_threshold), 0.f, 1.f);
    float alpha2  = lerp(0.0f, 1.0f, smoothstep(0.0f, 1.0f, factor2));

    if (alpha > 0.0f)
    {
        float2 xy0  = screen_position_at_pixel(float2(0.0f));
        int    minJ = std::max(0, int(-xy0.y / m_zoom));
        int    maxJ = std::min(m_image->size().y - 1, int(ceil((size().y - xy0.y) / m_zoom)));
        int    minI = std::max(0, int(-xy0.x / m_zoom));
        int    maxI = std::min(m_image->size().x - 1, int(ceil((size().x - xy0.x) / m_zoom)));

        for (int j = minJ; j <= maxJ; ++j)
        {
            for (int i = minI; i <= maxI; ++i)
            {
                auto   pos   = screen_position_at_pixel(float2(i + 0.5f, j + 0.5f));
                float4 pixel = image_pixel({i, j});

                static const vector<string> prefix{"R:", "G:", "B:", "A:"};
                static constexpr float3x4   cols{
                      {0.7f, 0.15f, 0.15f}, {0.1f, 0.5f, 0.1f}, {0.2f, 0.2f, 0.9f}, {0.8f, 0.8f, 0.8f}};
                if (alpha2 > 0.f)
                {
                    float2 c_pos = pos + float2{0.f, (-1 - 1.5f) * line_height};
                    auto   text  = fmt::format("({},{})", i, j);
                    draw_text(c_pos + int2{1, 2}, text, float4{0.f, 0.f, 0.f, alpha2}, font, align);
                    draw_text(c_pos, text, float4{cols[3], alpha2}, font, align);
                }

                for (int c = 0; c < 4; ++c)
                {
                    float2 c_pos = pos + float2{0.f, (c - 1.5f) * line_height};
                    float4 col{cols[c], alpha};
                    auto   text = fmt::format("{}{:>6.3f}", prefix[c], pixel[c]);
                    draw_text(c_pos + int2{1, 2}, text, float4{0.f, 0.f, 0.f, alpha}, font, align);
                    draw_text(c_pos, text, col, font, align);
                }
            }
        }
    }
}

void SampleViewer::draw_image_border() const
{
    if (!m_image || minelem(m_image->size()) == 0)
        return;

    ImGui::GetBackgroundDrawList()->AddRect(screen_position_at_pixel(float2(0.0f)),
                                            screen_position_at_pixel(float2{m_image->size()}),
                                            ImColor{0.5f, 0.5f, 0.5f, 1.0f}, 0.f, ImDrawFlags_None, 2.f);
}

void SampleViewer::draw_contents() const
{
    if (m_image && size().x > 0 && size().y > 0)
    {
        float2 randomness(std::generate_canonical<float, 10>(g_rand) * 255,
                          std::generate_canonical<float, 10>(g_rand) * 255);

        m_shader->set_uniform("randomness", randomness);
        m_shader->set_uniform("gain", powf(2.0f, m_exposure));
        m_shader->set_uniform("gamma", m_gamma);
        m_shader->set_uniform("sRGB", m_sRGB);
        m_shader->set_uniform("clamp_to_LDR", m_clamp_to_LDR);
        m_shader->set_uniform("do_dither", m_dither);

        m_shader->set_uniform("primary_pos", image_position());
        m_shader->set_uniform("primary_scale", image_scale());

        m_shader->set_uniform("blend_mode", 0); //(int)m_blend_mode);
        m_shader->set_uniform("channel", 0);    //(int)m_channel);
        m_shader->set_uniform("bg_mode", 2);    //(int)m_bg_mode);
        m_shader->set_uniform("bg_color", m_bg_color);

        // if (m_reference_image)
        // {
        //     float2 ref_pos, ref_scale;
        //     image_position_and_scale(ref_pos, ref_scale, m_reference_image);
        //     m_shader->set_uniform("has_reference", true);
        //     m_shader->set_uniform("secondary_pos", ref_pos);
        //     m_shader->set_uniform("secondary_scale", ref_scale);
        // }
        // else
        {
            m_shader->set_uniform("has_reference", false);
            m_shader->set_uniform("secondary_pos", float2(1.f, 1.f));
            m_shader->set_uniform("secondary_scale", float2(1.f, 1.f));
        }

        m_shader->begin();
        m_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 6, false);
        m_shader->end();
    }
}

void SampleViewer::draw_background()
{
    auto &io = ImGui::GetIO();

    process_hotkeys();

    try
    {
        //
        // calculate the viewport sizes
        // fbsize is the size of the window in pixels while accounting for dpi factor on retina screens.
        // for retina displays, io.DisplaySize is the size of the window in points (logical pixels)
        // but we need the size in pixels. So we scale io.DisplaySize by io.DisplayFramebufferScale
        int2 fbscale         = io.DisplayFramebufferScale;
        auto fbsize          = int2{io.DisplaySize} * fbscale;
        int2 viewport_offset = {0, 0};
        int2 viewport_size   = io.DisplaySize;
        if (auto id = m_params.dockingParams.dockSpaceIdFromName("MainDockSpace"))
        {
            auto central_node = ImGui::DockBuilderGetCentralNode(*id);
            viewport_size     = int2{int(central_node->Size.x), int(central_node->Size.y)};
            viewport_offset   = int2{int(central_node->Pos.x), int(central_node->Pos.y)};
        }

        if (!io.WantCaptureMouse)
        {
            auto p      = float2{io.MousePos};
            auto scroll = float2{io.MouseWheelH, io.MouseWheel};
#if defined(__EMSCRIPTEN__)
            scroll *= 10.0f;
#endif
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                set_pixel_at_position(p + float2{ImGui::GetMouseDragDelta(ImGuiMouseButton_Left)},
                                      pixel_at_position(p));
                ImGui::ResetMouseDragDelta();
            }
            else if (ImGui::IsKeyDown(ImGuiMod_Shift))
                // panning
                set_pixel_at_position(p + scroll * 4.f, pixel_at_position(p));
            else
                zoom_by(scroll.y / 4.f, p);
        }

        //
        // clear the framebuffer and set up the viewport
        //

        m_render_pass->resize(fbsize);
        m_render_pass->set_viewport({0, 0}, fbsize); //((viewport_offset)*fbscale, (viewport_size)*fbscale);

        m_render_pass->begin();

        draw_contents();
        draw_pixel_info();
        draw_pixel_grid();
        draw_image_border();

        m_render_pass->end();
    }
    catch (const std::exception &e)
    {
        fmt::print(stderr, "Drawing failed:\n\t{}.", e.what());
        HelloImGui::Log(HelloImGui::LogLevel::Error, "Drawing failed:\n\t%s.", e.what());
    }
}

void SampleViewer::process_hotkeys()
{
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (!m_image)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract))
        zoom_out();
    else if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))
        zoom_in();
    else if (ImGui::IsKeyPressed(ImGuiKey_E))
        m_exposure += ImGui::IsKeyDown(ImGuiMod_Shift) ? 0.25f : -0.25f;
    else if (ImGui::IsKeyPressed(ImGuiKey_G))
        m_gamma = std::max(0.02f, m_gamma + (ImGui::IsKeyDown(ImGuiMod_Shift) ? 0.02f : -0.02f));
    else if (ImGui::IsKeyPressed(ImGuiKey_F))
        fit();
    else if (ImGui::IsKeyPressed(ImGuiKey_C))
        center();
}

int main(int argc, char **argv)
{
    vector<string> args;
    bool           help                 = false;
    bool           error                = false;
    bool           launched_from_finder = false;

    try
    {
        for (int i = 1; i < argc; ++i)
        {
            if (strcmp("--help", argv[i]) == 0 || strcmp("-h", argv[i]) == 0)
                help = true;
            else if (strncmp("-psn", argv[i], 4) == 0)
                launched_from_finder = true;
            else
            {
                if (strncmp(argv[i], "-", 1) == 0)
                {
                    fmt::print(stderr, "Invalid argument: \"{}\"!\n", argv[i]);
                    help  = true;
                    error = true;
                }
                args.push_back(argv[i]);
            }
        }
        (void)launched_from_finder;
    }
    catch (const std::exception &e)
    {
        fmt::print(stderr, "Error: {}\n", e.what());
        help  = true;
        error = true;
    }
    if (help)
    {
        fmt::print(error ? stderr : stdout, R"(Syntax: {} [options]
Options:
   -h, --help                Display this message
)",
                   argv[0]);
        return error ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    try
    {
        SampleViewer viewer;
        viewer.run();
    }
    catch (const std::runtime_error &e)
    {
        fmt::print(stderr, "Caught a fatal error: {}\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
