struct Light {
    vec3 color;
    float energy;
    vec3 position;
    float distance;
};


layout (std140) uniform Lights {
    int count;
    int _x, _y, _z;
    Light lights[8];  
};


vec3 calc_point_light(Light light, vec3 position, vec3 normal) {
    vec3 light_vector = light.position - position;
    vec3 light_dir = normalize(light_vector);
    float diffuse = max(dot(normal, light_dir), 0.0);
    float distance = length(light_vector);
    // Inverse square falloff:
    //     constant + linear * distance + quadratic * distance * distance
    // Default values from Blender are:
    //   - constant = 0.0
    //   - linear = 0.0
    //   - quadratic = 1.0
    // Therefore, we can simplify the equation to:
    //     distance * distance
    float attenuation = 1.0 / (distance * distance);
    return light.color * light.energy * diffuse * attenuation;
}
