using namespace metal;

struct VertexOut {
    float4 position [[position]];
};

fragment float4 fragment_main(VertexOut vert [[stage_in]],
                                constant float4 &clear_color) {
    return clear_color;
}