precision mediump float;

in vec2  position;
out vec2 uv;

void main()
{
    gl_Position = vec4(0.75 * position, 0.5, 1.0);
    uv          = position;
}