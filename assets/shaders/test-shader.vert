precision mediump float;

uniform float clear_depth;
in vec2       position;

void main()
{
    gl_Position = vec4(0.75 * position, clear_depth, 1.0);
}