#version 330 core


#include "matrices.glsl"


#include "attributes.glsl"


out vec3 inout_normal;
out vec4 inout_color;
out vec2 inout_uv;


#include "psx_artifacts.glsl"


void main() {
    vec3 mv_position = vec3(view * model * vec4(in_position, 1.0));
    vec3 mv_normal = normalize(vec3(view * model * vec4(in_normal, 0.0)));
    vec3 view_vector = vec3(0, 0, 1);
    float diffuse = max(dot(mv_normal, view_vector), 0.2);
    diffuse = diffuse * 10 * (1 / length(mv_position));
    
    gl_Position = snap(in_position);
    inout_normal = in_normal;
    inout_color = vec4(vec3(diffuse), 1.0);
    inout_uv = in_uv;
}
