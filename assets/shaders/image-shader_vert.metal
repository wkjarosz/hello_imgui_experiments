using namespace metal;

struct VertexOut
{
    float4 position [[position]];
    float2 primary_uv;
};

vertex VertexOut vertex_main(const device float2 *position,
                             constant float2 &primary_scale,
                             constant float2 &primary_pos,
                            uint id [[vertex_id]]) {
    VertexOut vert = {};
    vert.primary_uv = 2.5*(((position[id] / float2(2.0)) - primary_pos) + float2(0.5)) / primary_scale;
    // vert.secondary_uv = (((position[id] / float2(2.0)) - secondary_pos) + float2(0.5)) / secondary_scale;
    vert.position = float4(position[id].x, -position[id].y, 0.0, 1.0);
    return vert;
}