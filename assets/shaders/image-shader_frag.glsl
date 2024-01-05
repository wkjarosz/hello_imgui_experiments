precision mediump float;

out vec4          frag_color;
in vec2           uv;
uniform sampler2D image;

void main()
{
    vec4 img   = texture(image, uv);
    frag_color = img; // vec4(2.0 * uv.x, 2.0 * uv.y, 0.0, 1.0);
}
