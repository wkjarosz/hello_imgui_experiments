#include "texture.h"
#include <memory>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

Texture::Texture(PixelFormat pixel_format, ComponentFormat component_format, const int2 &size,
                 InterpolationMode min_interpolation_mode, InterpolationMode mag_interpolation_mode, WrapMode wrap_mode,
                 uint8_t samples, uint8_t flags, bool manual_mipmapping) :
    m_pixel_format(pixel_format),
    m_component_format(component_format), m_min_interpolation_mode(min_interpolation_mode),
    m_mag_interpolation_mode(mag_interpolation_mode), m_wrap_mode(wrap_mode), m_samples(samples), m_flags(flags),
    m_size(size), m_manual_mipmapping(manual_mipmapping)
{
    init();
}

Texture::Texture(const std::string &filename, InterpolationMode min_interpolation_mode,
                 InterpolationMode mag_interpolation_mode, WrapMode wrap_mode) :
    m_component_format(ComponentFormat::Float32),
    m_min_interpolation_mode(min_interpolation_mode), m_mag_interpolation_mode(mag_interpolation_mode),
    m_wrap_mode(wrap_mode), m_samples(1), m_flags(TextureFlags::ShaderRead), m_manual_mipmapping(false)
{
    int n = 0;
    stbi_ldr_to_hdr_scale(1.0f);
    stbi_ldr_to_hdr_gamma(1.0f);
    using Holder = std::unique_ptr<float[], void (*)(void *)>;
    Holder texture_data(stbi_loadf(filename.c_str(), &m_size.x, &m_size.y, &n, 4), stbi_image_free);
    if (!texture_data)
        throw std::runtime_error("Could not load texture data from file \"" + filename +
                                 "\". Reason: " + stbi_failure_reason());

    n = 4;
    switch (n)
    {
    case 1: m_pixel_format = PixelFormat::R; break;
    case 2: m_pixel_format = PixelFormat::RA; break;
    case 3: m_pixel_format = PixelFormat::RGB; break;
    case 4: m_pixel_format = PixelFormat::RGBA; break;
    default: throw std::runtime_error("Texture::Texture(): unsupported channel count!");
    }
    PixelFormat pixel_format = m_pixel_format;
    init();
    if (m_pixel_format != pixel_format)
        throw std::runtime_error("Texture::Texture(): pixel format not supported by the hardware!");
    upload((const uint8_t *)texture_data.get());
}

Texture::Texture(const std::string &filename, std::string_view data, InterpolationMode min_interpolation_mode,
                 InterpolationMode mag_interpolation_mode, WrapMode wrap_mode) :
    m_component_format(ComponentFormat::Float32),
    m_min_interpolation_mode(min_interpolation_mode), m_mag_interpolation_mode(mag_interpolation_mode),
    m_wrap_mode(wrap_mode), m_samples(1), m_flags(TextureFlags::ShaderRead), m_manual_mipmapping(false)
{
    int n = 0;
    stbi_ldr_to_hdr_scale(1.0f);
    stbi_ldr_to_hdr_gamma(1.0f);
    using Holder = std::unique_ptr<float[], void (*)(void *)>;

    Holder texture_data(
        stbi_loadf_from_memory((const stbi_uc *)data.begin(), data.length(), &m_size.x, &m_size.y, &n, 4),
        stbi_image_free);
    if (!texture_data)
        throw std::runtime_error("Could not load texture data from file \"" + filename +
                                 "\". Reason: " + stbi_failure_reason());

    n = 4;
    switch (n)
    {
    case 1: m_pixel_format = PixelFormat::R; break;
    case 2: m_pixel_format = PixelFormat::RA; break;
    case 3: m_pixel_format = PixelFormat::RGB; break;
    case 4: m_pixel_format = PixelFormat::RGBA; break;
    default: throw std::runtime_error("Texture::Texture(): unsupported channel count!");
    }
    PixelFormat pixel_format = m_pixel_format;
    init();
    if (m_pixel_format != pixel_format)
        throw std::runtime_error("Texture::Texture(): pixel format not supported by the hardware!");
    upload((const uint8_t *)texture_data.get());
}

size_t Texture::bytes_per_pixel() const
{
    size_t result = 0;
    switch (m_component_format)
    {
    case ComponentFormat::UInt8: result = 1; break;
    case ComponentFormat::Int8: result = 1; break;
    case ComponentFormat::UInt16: result = 2; break;
    case ComponentFormat::Int16: result = 2; break;
    case ComponentFormat::UInt32: result = 4; break;
    case ComponentFormat::Int32: result = 4; break;
    case ComponentFormat::Float16: result = 2; break;
    case ComponentFormat::Float32: result = 4; break;
    default: throw std::runtime_error("Texture::bytes_per_pixel(): invalid component format!");
    }

    return result * channels();
}

size_t Texture::channels() const
{
    size_t result = 1;
    switch (m_pixel_format)
    {
    case PixelFormat::R: result = 1; break;
    case PixelFormat::RA: result = 2; break;
    case PixelFormat::RGB: result = 3; break;
    case PixelFormat::RGBA: result = 4; break;
    case PixelFormat::BGR: result = 3; break;
    case PixelFormat::BGRA: result = 4; break;
    case PixelFormat::Depth: result = 1; break;
    case PixelFormat::DepthStencil: result = 2; break;
    default: throw std::runtime_error("Texture::channels(): invalid pixel format!");
    }
    return result;
}
