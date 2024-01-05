using namespace metal;

struct VertexOut
{
    float4 position [[position]];
    float2 uv;
};

vertex VertexOut vertex_main(const device float2 *position,
                            uint id [[vertex_id]]) {
    VertexOut vert;
    vert.position = float4(position[id], 0.5, 1.0);
    vert.uv = (position[id] + 1.0) * 0.5;
    vert.uv.y = 1.0 - vert.uv.y;
    return vert;
}
