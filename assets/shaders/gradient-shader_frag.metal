using namespace metal;

struct VertexOut
{
    float4 position [[position]];
    float2 uv;
};

fragment float4 fragment_main(VertexOut vert [[stage_in]])
{
    return float4(2.0*vert.uv.x, 2.0*vert.uv.y, 0.0, 1.0);
}