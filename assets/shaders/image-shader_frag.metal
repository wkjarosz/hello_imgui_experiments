#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#include "colormaps.metal"
#include "colorspaces.metal"

struct VertexOut
{
    float4 position [[position]];
    float2 uv;
};

fragment float4 fragment_main(VertexOut vert [[stage_in]],
                              texture2d<float, access::sample> image,
                              sampler image_sampler)
{
    float4 img = image.sample(image_sampler, vert.uv);
    return img;// * float4(2.0*vert.uv.x, 2.0*vert.uv.y, 0.0, 1.0);
}