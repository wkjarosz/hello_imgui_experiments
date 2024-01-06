/** \file SampleViewer.cpp
    \author Wojciech Jarosz
*/

#include "app.h"

#include "hello_imgui/hello_imgui.h"
#include "hello_imgui/hello_imgui_include_opengl.h" // cross-platform way to include OpenGL headers
#include "imgui_ext.h"
#include "imgui_internal.h"

#include "opengl_check.h"

#include "texture.h"
#include "timer.h"

#include <cmath>
#include <fmt/core.h>
#include <fstream>
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

using namespace linalg::ostream_overloads;

using std::pair;
using std::to_string;

#ifdef __EMSCRIPTEN__
EM_JS(int, screen_width, (), { return screen.width; });
EM_JS(int, screen_height, (), { return screen.height; });
EM_JS(int, window_width, (), { return window.innerWidth; });
EM_JS(int, window_height, (), { return window.innerHeight; });
#endif

SampleViewer::SampleViewer()
{
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

    m_bg_color                                 = float4{0.3, 0.3, 0.3, 1.0};
    m_params.imGuiWindowParams.backgroundColor = m_bg_color;

    m_params.callbacks.LoadAdditionalFonts = [this]()
    {
        std::string roboto_r = "fonts/Roboto/Roboto-Regular.ttf";
        std::string roboto_b = "fonts/Roboto/Roboto-Bold.ttf";
        if (!HelloImGui::AssetExists(roboto_r) || !HelloImGui::AssetExists(roboto_b))
            return;

        for (auto font_size : {14, 10, 16, 18, 30})
        {
            m_regular[font_size] = HelloImGui::LoadFontTTF_WithFontAwesomeIcons(roboto_r, (float)font_size);
            m_bold[font_size]    = HelloImGui::LoadFontTTF_WithFontAwesomeIcons(roboto_b, (float)font_size);
        }
    };

    m_params.callbacks.SetupImGuiStyle = [this]()
    {
        try
        {
            m_render_pass = new RenderPass(false, true);
            m_render_pass->set_cull_mode(RenderPass::CullMode::Disabled);
            m_render_pass->set_depth_test(RenderPass::DepthTest::Always, false);
            // m_shader =
            //     new Shader(m_render_pass, "Test shader", Shader::from_asset("shaders/gradient-shader_vert"),
            //                Shader::from_asset("shaders/gradient-shader_frag"), Shader::BlendMode::AlphaBlend);
            m_shader     = new Shader(m_render_pass, "Test shader", Shader::from_asset("shaders/image-shader_vert"),
                                      Shader::prepend_includes(Shader::from_asset("shaders/image-shader_frag"),
                                                               {"shaders/colorspaces", "shaders/colormaps"}),
                                      Shader::BlendMode::AlphaBlend);
            m_null_image = new Texture(Texture::PixelFormat::RGBA, Texture::ComponentFormat::Float32, {1, 1},
                                       Texture::InterpolationMode::Nearest, Texture::InterpolationMode::Nearest,
                                       Texture::WrapMode::Repeat);
            static float pixel[] = {0.5f, 0.5f, 0.5f, 1.f};
            m_null_image->upload((const uint8_t *)&pixel);
            m_shader->set_texture("image", m_null_image);

            const float positions[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f, -1.f, 1.f};
            m_shader->set_buffer("position", VariableType::Float32, {6, 2}, positions);
            m_shader->set_uniform("primary_pos", float2{0.f});
            m_shader->set_uniform("primary_scale", float2{1.f});

            HelloImGui::Log(HelloImGui::LogLevel::Info, "Successfully initialized GL!");
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
#ifndef __EMSCRIPTEN__
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open image..."))
        {
            auto result = pfd::open_file("Open image", "", {"Image files", "*.png *.jpeg *.jpg *.hdr *.bmp"}).result();
            if (!result.empty())
            {
                HelloImGui::Log(HelloImGui::LogLevel::Debug, "Loading file '%s'...", result.front().c_str());
                Texture *tex = new Texture(result.front());
                m_shader->set_texture("image", tex);
            }
        }
#else
        static Texture *tex = nullptr;
        ;
        auto handle_upload_file =
            [](const string &filename, const string &mime_type, string_view buffer, void *my_data = nullptr)
        {
            auto that{reinterpret_cast<SampleViewer *>(my_data)};
            HelloImGui::Log(HelloImGui::LogLevel::Debug, "Loading file '%s' of mime type '%s' ...", filename.c_str(),
                            mime_type.c_str());
            delete tex;
            tex = new Texture(filename, buffer);
            that->m_shader->set_texture("image", tex);
            delete tex;
            tex = nullptr;
        };
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open image..."))
        {
            // open the browser's file selector, and pass the file to the upload handler
            emscripten_browser_file::upload(".png,.hdr,.jpg,.jpeg", handle_upload_file, this);
            HelloImGui::Log(HelloImGui::LogLevel::Debug, "Requesting file from user");
        }
#endif
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

void SampleViewer::draw_background()
{
    auto &io = ImGui::GetIO();

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

        //
        // clear the framebuffer and set up the viewport
        //

        m_bg_color = float4{1.0, 1.5, 1.5, 1.0};
        // m_params.imGuiWindowParams.backgroundColor = m_bg_color;

        m_render_pass->resize(fbsize);

        // m_render_pass->set_viewport((viewport_offset)*fbscale, (viewport_size)*fbscale);
        m_render_pass->set_viewport({0, 0}, fbsize);
        // m_render_pass->set_clear_color(float4{fmod(frame++ / 100.f, 1.f), 0.2, 0.1, 1.0});
        m_render_pass->set_clear_color(m_bg_color);

        m_render_pass->begin();

        // m_shader->begin();
        // m_shader->draw_array(Shader::PrimitiveType::TriangleStrip, 0, 4, false);
        // m_shader->end();
        m_shader->begin();
        m_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 6, false);
        m_shader->end();

        m_render_pass->end();
    }
    catch (const std::exception &e)
    {
        fmt::print(stderr, "Drawing failed:\n\t{}.", e.what());
        HelloImGui::Log(HelloImGui::LogLevel::Error, "Drawing failed:\n\t%s.", e.what());
    }
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
