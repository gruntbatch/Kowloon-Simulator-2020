#pragma once


#include "numbers.h"


union Vector2 {
    struct { f32 x, y; };
    struct { f32 u, v; };
    struct { f32 s, t; };
};


union Vector2 Vector2(f32 x, f32 y);
union Vector2 Add2(union Vector2 l, union Vector2 r);
union Vector2 DirectionFrom2(union Vector2 here, union Vector2 there);
f32 Distance2(union Vector2 l, union Vector2 r);
f32 DistanceSquared2(union Vector2 l, union Vector2 r);
bool Equal2(union Vector2 l, union Vector2 r);
f32 Magnitude2(union Vector2 v);
f32 MagnitudeSquared2(union Vector2 v);
union Vector2 Negate2(union Vector2 v);
union Vector2 Normalize2(union Vector2 v);
union Vector2 Scale2(union Vector2 v, f32 scalar);
union Vector2 Sub2(union Vector2 l, union Vector2 r);


union Vector3 {
    struct { f32 x, y, z; };
    struct { f32 r, g, b; };
    struct { union Vector2 xy; };
};


union Vector3 Vector3(f32 x, f32 y, f32 z);
union Vector3 Add3(union Vector3 l, union Vector3 r);
union Vector3 Cross3(union Vector3 l, union Vector3 r);
f32 Dot3(union Vector3 l, union Vector3 r);
union Vector3 Negate3(union Vector3 v);
union Vector3 Normalize3(union Vector3 v);
f32 Magnitude3(union Vector3 v);
f32 MagnitudeSquared3(union Vector3 v);
union Vector3 Scale3(union Vector3 v, f32 scalar);
union Vector3 Sub3(union Vector3 l, union Vector3 r);


union Vector4 {
    struct { f32 x, y, z, w; };
    struct { union Vector2 xy, zw; };
    struct { union Vector3 xyz; };

    struct { f32 r, g, b, a; };
    struct { union Vector3 rgb; };
    
    struct { f32 u, v, s, t; };
    struct { union Vector2 uv, st; };
    f32 floats[4];
};


union Vector4 Vector4 (f32 x, f32 y, f32 z, f32 w);


union Quaternion {
    struct { f32 x, y, z, w; };
};


union Quaternion Quaternion(void);
union Quaternion AxisAngle(union Vector3 axis, f32 radians);
f32 DotQ(union Quaternion l, union Quaternion r);
union Quaternion NormalizeQ(union Quaternion q);
union Quaternion MulQ(union Quaternion l, union Quaternion r);


union IVector2 {
    struct { i32 x, y; };
    struct { i32 width, height ; };
};


union IVector2 IVector2(i32 x, i32 y);
union IVector2 AddI2(union IVector2 l, union IVector2 r);
union IVector2 NormalizeI2(union IVector2 v);


union UVector4 {
    struct { u32 x, y, z, w; };
};


union Matrix4 {
    union Vector4 vectors[4];
    f32 columns[4][4]; /* Column-major */
    f32 floats[16];
};


union Matrix4 Matrix4(f32 diagonal);
union Matrix4 InvertM4(union Matrix4 m);
union Matrix4 LookAt(union Vector3 eye, union Vector3 target, union Vector3 up);
union Matrix4 MulM4(union Matrix4 l, union Matrix4 r);
union Matrix4 Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
union Matrix4 Perspective(f32 fov, f32 aspect, f32 near, f32 far);
union Matrix4 Rotation(union Quaternion q);
union Matrix4 Scale(union Vector3 v);
union Matrix4 Transformation(union Vector3 translation, union Quaternion rotation, union Vector3 scale);
union Vector4 Transform4(union Matrix4 l, union Vector4 r);
union Matrix4 Translation(union Vector3 v);


union Rect {
    struct { f32 x, y, width, height; };
    struct { union Vector2 origin, size; };
};


union Rect Rect(f32 x, f32 y, f32 width, f32 height);



union IRect {
    struct { i32 x, y, width, height; };
    struct { union IVector2 origin, size; };
};


union IRect IRect(i32 x, i32 y, i32 width, i32 height);


f32 Value1(f32 point);
f32 Value2(union Vector2 point);
f32 Voroni2(union Vector2 point, f32 scale);
f32 FastVoroni2(union Vector2 point, f32 scale);
