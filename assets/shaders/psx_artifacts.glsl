// TODO Rename this file to something more meaningful and useful.
// TODO Supply the correct snap values via a uniform buffer.
// TODO Warp texture coordinates.
// 320 / 2
#define SNAP_X 160
// 240 / 2
#define SNAP_Y 120

vec4 snap(vec3 i) {
    vec4 snap_to_pixel = projection * view * model * vec4(i, 1.0);
    vec4 vertex = snap_to_pixel;
    vertex.xyz = snap_to_pixel.xyz / snap_to_pixel.w;
    vertex.x = floor(SNAP_X * vertex.x) / SNAP_X;
    vertex.y = floor(SNAP_Y * vertex.y) / SNAP_Y;
    vertex.xyz *= snap_to_pixel.w;
    return vertex;
}
