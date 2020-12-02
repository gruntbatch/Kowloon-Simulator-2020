#version 330 core


#include "attributes.glsl"


out vec3 inout_normal;
out vec4 inout_color;
out vec2 inout_uv;


void main() {
    gl_Position = vec4(in_position, 1.0);
    inout_normal = in_normal;
    inout_color = in_color;
    inout_uv = in_uv;
}
