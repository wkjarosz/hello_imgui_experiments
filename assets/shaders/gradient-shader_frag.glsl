precision mediump float;

out vec4 frag_color;
in vec2  uv;

void main()
{
    frag_color = vec4(2.0 * uv.x, 2.0 * uv.y, 0.0, 1.0);
}
