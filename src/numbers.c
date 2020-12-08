#include "numbers.h"


#include <stdlib.h>


f32 to_radians(f32 degrees) {
    return degrees * RADIANS;
}


f32 to_degrees(f32 radians) {
    return radians * DEGREES;
}


f32 clampf(f32 min, f32 f, f32 max) {
    const f32 t = (f < min) ? min : f;
    return (t > max) ? max : t;
}


f32 lerpf(f32 a, f32 b, f32 f) {
    return (a * (1.0f - f)) + (b * f);
}


f32 randf(f32 min, f32 max) {
    return ((f32) rand() / (f32) RAND_MAX) * (max - min) + min;
}


int nearest_pot(int minimum) {
    int maximum = 2;
    
    while (maximum < minimum) {
        maximum = 2 * maximum;
    }
    
    return maximum;
}


int is_pot(int number) {
    if (number == 0) {
        return 1;
    }

    int test = 1;
    while (test <= number) {
        if (test == number) {
            return 1;
        }
        test = 2 * test;
    }
    
    return 0;
}
