precision mediump float;

in vec2  position;
out vec2 uv;

void main()
{
    gl_Position = vec4(position, 0.5, 1.0);
    uv          = (position + 1.0) * 0.5;
    uv.y        = 1.0 - uv.y;
}