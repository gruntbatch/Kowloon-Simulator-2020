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


#define ENERGY_SCALAR 10.0


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
    // ENERGY_SCALAR brings our attenuation calculation in line
    // with Blender's calculation. I'm not entirely sure if this
    // is an accurate way of doing it, but it seems to match
    // well enough.
    float attenuation = 1.0 / (ENERGY_SCALAR * (distance * distance));
    return light.color * light.energy * diffuse * attenuation;
}
