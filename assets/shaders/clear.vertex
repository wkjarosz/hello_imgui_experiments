using namespace metal;

struct VertexOut {
    float4 position [[position]];
};

vertex VertexOut vertex_main(const device float2 *position,
                                constant float &clear_depth,
                                uint id [[vertex_id]]) {
    VertexOut vert;
    vert.position = float4(position[id], clear_depth, 1.f);
    return vert;
}