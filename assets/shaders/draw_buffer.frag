#version 330 core


in vec2 inout_uv;


uniform sampler2D uni_texture;


out vec4 out_color;


void main() {
    out_color = texture(uni_texture, inout_uv);
}
