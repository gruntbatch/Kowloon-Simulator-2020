#version 330 core


#include "matrices.glsl"


#include "lights.glsl"


#include "attributes.glsl"


out vec3 inout_normal;
out vec4 inout_color;
out vec2 inout_uv;


#include "psx_artifacts.glsl"


void main() {
    vec3 m_position = vec3(model * vec4(in_position, 1.0));
    vec3 m_normal = normalize(vec3(model * vec4(in_normal, 0.0)));
    vec3 color = vec3(0);
    for (int i=0; i<count; i++) {
        color += calc_point_light(lights[i], m_position, m_normal);
    }

    gl_Position = snap(in_position);
    inout_normal = in_normal;
    // inout_color = vec4(vec3(diffuse), 1.0);
    inout_color = vec4(color, 1.0);
    inout_uv = in_uv;
}
