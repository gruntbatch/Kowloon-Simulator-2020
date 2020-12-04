#pragma once


#include <limits.h>
#include <math.h>
#include <stdint.h>


#define TRUE 1
#define FALSE 0
typedef uint8_t bool;


typedef int8_t i8;
#define I8_MIN INT8_MIN
#define I8_MAX INT8_MAX
typedef int16_t i16;
#define I16_MIN INT16_MIN
#define I16_MAX INT16_MAX
typedef int32_t i32;
#define I32_MIN INT32_MIN
#define I32_MAX INT32_MAX


typedef uint8_t u8;
#define U8_MAX UINT8_MAX
typedef uint16_t u16;
#define U16_MAX UINT16_MAX
#define U24_MAX 0xFFFFFF
typedef uint32_t u32;
#define U32_MAX UINT32_MAX


typedef float f32;
typedef double f64;


#define PI (3.14159265358979323846)
#define PI2 (2.0 * PI)

#define RADIANS (PI / 180.0f)
#define TO_RADIANS(degrees) (degrees * RADIANS)

#define DEGREES (180.0f / PI)
#define TO_DEGREES(radians) (radians * DEGREES)


#define SQUARE(x) (x * x)


f32 to_radians(f32 degrees);
f32 to_degrees(f32 radians);


f32 clampf(f32 f, f32 min, f32 max);


f32 lerpf(f32 a, f32 b, f32 f);
f32 randf(f32 min, f32 max);


int nearest_pot(int minimum);
int is_pot(int number);
