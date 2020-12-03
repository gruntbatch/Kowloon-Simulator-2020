#include "mathematics.h"


#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


union Vector2 Vector2(f32 x, f32 y) {
    return (union Vector2) { .x=x, .y=y };
}


union Vector2 Add2(union Vector2 l, union Vector2 r) {
    return (union Vector2) { .x=l.x + r.x,
                             .y=l.y + r.y };
}


union Vector2 DirectionFrom2(union Vector2 here, union Vector2 there) {
    return Normalize2(Sub2(there, here));
}


f32 Distance2(union Vector2 l, union Vector2 r) {
    return Magnitude2(Sub2(l, r));
}


f32 DistanceSquared2(union Vector2 l, union Vector2 r) {
    return MagnitudeSquared2(Sub2(l, r));
}


f32 Dot2(union Vector2 l, union Vector2 r) {
    return l.x * r.x + l.y * r.y;
}


bool Equal2(union Vector2 l, union Vector2 r) {
    return (l.x == r.x && l.y == r.y);
}


f32 Magnitude2(union Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}


f32 MagnitudeSquared2(union Vector2 v) {
    return v.x * v.x + v.y * v.y;
}


union Vector2 Negate2(union Vector2 v) {
    return (union Vector2) { .x=-v.x, .y=-v.y };
}


union Vector2 Normalize2(union Vector2 in) {
    f32 dot = in.x * in.x + in.y * in.y;
    f32 magnitude = sqrtf(dot);
    
    union Vector2 out = Vector2(0.0f, 0.0f);
    if (magnitude) {
        out.x = in.x / magnitude;
        out.y = in.y / magnitude;
    }
    return out;
}


union Vector2 Scale2(union Vector2 in, f32 scalar) {
    return (union Vector2) { .x=in.x * scalar,
                             .y=in.y * scalar };
}


union Vector2 Sub2(union Vector2 l, union Vector2 r) {
    return (union Vector2) { .x=l.x - r.x,
                             .y=l.y - r.y };
}


struct Line2 Line2(union Vector2 a, union Vector2 b) {
    return (struct Line2) { .a=a, .b=b };
}


union Vector2 Intersect2(struct Line2 l, struct Line2 r) {
    union Vector2 a = l.a;
    union Vector2 b = l.b;
    
    // TODO This is magic to me
    float a1 = b.y - a.y;
    float b1 = a.x - b.x;

    float a2 = b.y - a.y;
    float b2 = a.x - b.x;

    float d = a1 * b2 - a2 * b1;

    if (d == 0) {
	return Vector2(INFINITY, INFINITY);
    } else {
	float c1 = a1 * (a.x) + b1 * (a.y);
	float c2 = a2 * (a.x) + b2 * (a.y);
	float x = (b2 * c1 - b1 * c2) / d;
	float y = (a1 * c2 - a2 * c1) / d;
	return Vector2(x, y);
    }
}


float DistanceToLine2(union Vector2 p, struct Line2 l) {
    /* Adapted from https://stackoverflow.com/a/1501725 */
    // Return minimum distance between line segment ab and point p
    union Vector2 a = l.a;
    union Vector2 b = l.b;
    union Vector2 nearest;
    float dist;

    float mag2 = DistanceSquared2(a, b); // i.e. |w-v|^2 -  avoid a sqrt
    if (mag2 == 0.0) {
	nearest = a;
	dist = Distance2(p, a);
    } else {
	// Consider the line extending the segment, parameterized as v + t (w - v).
	// We find projection of point p onto the line. 
	// It falls where t = [(p-v) . (w-v)] / |w-v|^2
	// We clamp t from [0,1] to handle points outside the segment vw.
	float t = fmaxf(0, fminf(1, Dot2(Sub2(p, a), Sub2(b, a)) / mag2));
	nearest = Add2(a, Scale2(Sub2(b, a), t)); // Nearest falls on the segment
	dist = Distance2(p, nearest);
    }

    return dist;    
}


float Sign2(union Vector2 p, struct Line2 l) {
    union Vector2 a = l.a;
    union Vector2 b = l.b;
    return (p.x - b.x) * (a.y - b.y) - (a.x - b.x) * (p.y - b.y);
}


union Vector3 Vector3(f32 x, f32 y, f32 z) {
    return (union Vector3) { .x=x, .y=y, .z=z };
}


union Vector3 Add3(union Vector3 l, union Vector3 r) {
    return (union Vector3) { .x=l.x + r.x,
                             .y=l.y + r.y,
                             .z=l.z + r.z };
}


union Vector3 Cross3(union Vector3 l, union Vector3 r) {
    return (union Vector3) { .x=(l.y * r.z) - (l.z * r.y),
                             .y=(l.z * r.x) - (l.x * r.z),
                             .z=(l.x * r.y) - (l.y * r.x) };
}


f32 Dot3(union Vector3 l, union Vector3 r) {
    return l.x * r.x + l.y * r.y + l.z * r.z;
}


union Vector3 Negate3(union Vector3 v) {
    return (union Vector3) { .x=-v.x, .y=-v.y, .z=-v.z };
}


union Vector3 Normalize3(union Vector3 v) {
    f32 magnitude = Magnitude3(v);

    union Vector3 out = Vector3(0.0f, 0.0f, 0.0f);
    if (magnitude) {
        out.x = v.x / magnitude;
        out.y = v.y / magnitude;
        out.z = v.z / magnitude;
    }
    return out;
}


f32 Magnitude3(union Vector3 v) {
    return sqrtf(Dot3(v, v));
}


f32 MagnitudeSquared3(union Vector3 v) {
    return Dot3(v, v);
}


union Vector3 Scale3(union Vector3 v, f32 scalar) {
    return (union Vector3) { .x=v.x * scalar,
                             .y=v.y * scalar,
                             .z=v.z * scalar };
}


union Vector3 Sub3(union Vector3 l, union Vector3 r) {
    return (union Vector3) { .x=l.x - r.x,
                             .y=l.y - r.y,
                             .z=l.z - r.z };
}


union Vector4 Vector4(f32 x, f32 y, f32 z, f32 w) {
    return (union Vector4) { .x=x, .y=y, .z=z, .w=w };
}


union Quaternion Quaternion(void) {
    return (union Quaternion) { .x=0.0f, .y=0.0f, .z=0.0f, .w=1.0f };
}


union IVector2 IVector2(i32 x, i32 y) {
    return (union IVector2) { .x=x, .y=y };
}


union IVector2 AddI2(union IVector2 l, union IVector2 r) {
    return (union IVector2) { .x=l.x + r.x,
            .y=l.y + r.y };
}


union IVector2 NormalizeI2(union IVector2 v) {
    return (union IVector2) { .x=(v.x < 0) ? -1 : 1,
            .y=(v.y < 0) ? -1 : 1 };
}


union Quaternion AxisAngle(union Vector3 axis, f32 radians) {
    axis = Normalize3(axis);
    f32 s = sinf(radians / 2.0f);
    
    return (union Quaternion) { .x=axis.x * s,
            .y=axis.y * s,
            .z=axis.z * s,
            .w=cosf(radians / 2.0f) };
}


f32 DotQ(union Quaternion l, union Quaternion r) {
    return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w;
}


union Quaternion NormalizeQ(union Quaternion q) {
    f32 magnitude = sqrtf(DotQ(q, q));

    union Quaternion out = { .x=0, .y=0, .z=0, .w=0 };
    if (magnitude) {
        out.x = q.x / magnitude;
        out.y = q.y / magnitude;
        out.z = q.z / magnitude;
        out.w = q.w / magnitude;
    }
    return out;
}


union Quaternion MulQ(union Quaternion l, union Quaternion r) {
    return (union Quaternion) { .x=( l.x * r.w) + (l.y * r.z) - (l.z * r.y) + (l.w * r.x),
                                .y=(-l.x * r.z) + (l.y * r.w) + (l.z * r.x) + (l.w * r.y),
                                .z=( l.x * r.y) - (l.y * r.x) + (l.z * r.w) + (l.w * r.z),
                                .w=(-l.x * r.x) - (l.y * r.y) - (l.z * r.z) + (l.w * r.w) };
}


union Matrix4 Matrix4(f32 diagonal) {
    return (union Matrix4) { .columns= { { diagonal, 0, 0, 0 },
                                         { 0, diagonal, 0, 0 },
                                         { 0, 0, diagonal, 0 },
                                         { 0, 0, 0, diagonal } } };
}


union Matrix4 InvertM4(union Matrix4 m) {

	union Matrix4 r;

	f32 s0 = m.columns[0][0] * m.columns[1][1] - m.columns[1][0] * m.columns[0][1];
	f32 s1 = m.columns[0][0] * m.columns[1][2] - m.columns[1][0] * m.columns[0][2];
	f32 s2 = m.columns[0][0] * m.columns[1][3] - m.columns[1][0] * m.columns[0][3];
	f32 s3 = m.columns[0][1] * m.columns[1][2] - m.columns[1][1] * m.columns[0][2];
	f32 s4 = m.columns[0][1] * m.columns[1][3] - m.columns[1][1] * m.columns[0][3];
	f32 s5 = m.columns[0][2] * m.columns[1][3] - m.columns[1][2] * m.columns[0][3];

	f32 c5 = m.columns[2][2] * m.columns[3][3] - m.columns[3][2] * m.columns[2][3];
	f32 c4 = m.columns[2][1] * m.columns[3][3] - m.columns[3][1] * m.columns[2][3];
	f32 c3 = m.columns[2][1] * m.columns[3][2] - m.columns[3][1] * m.columns[2][2];
	f32 c2 = m.columns[2][0] * m.columns[3][3] - m.columns[3][0] * m.columns[2][3];
	f32 c1 = m.columns[2][0] * m.columns[3][2] - m.columns[3][0] * m.columns[2][2];
	f32 c0 = m.columns[2][0] * m.columns[3][1] - m.columns[3][0] * m.columns[2][1];

	/* Should check for 0 determinant */
	f32 invdet = \
		1.0f / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);

	r.columns[0][0] = (m.columns[1][1] * c5 - m.columns[1][2] * c4 + m.columns[1][3] * c3) * invdet;
	r.columns[0][1] = (-m.columns[0][1] * c5 + m.columns[0][2] * c4 - m.columns[0][3] * c3) * invdet;
	r.columns[0][2] = (m.columns[3][1] * s5 - m.columns[3][2] * s4 + m.columns[3][3] * s3) * invdet;
	r.columns[0][3] = (-m.columns[2][1] * s5 + m.columns[2][2] * s4 - m.columns[2][3] * s3) * invdet;

	r.columns[1][0] = (-m.columns[1][0] * c5 + m.columns[1][2] * c2 - m.columns[1][3] * c1) * invdet;
	r.columns[1][1] = (m.columns[0][0] * c5 - m.columns[0][2] * c2 + m.columns[0][3] * c1) * invdet;
	r.columns[1][2] = (-m.columns[3][0] * s5 + m.columns[3][2] * s2 - m.columns[3][3] * s1) * invdet;
	r.columns[1][3] = (m.columns[2][0] * s5 - m.columns[2][2] * s2 + m.columns[2][3] * s1) * invdet;

	r.columns[2][0] = (m.columns[1][0] * c4 - m.columns[1][1] * c2 + m.columns[1][3] * c0) * invdet;
	r.columns[2][1] = (-m.columns[0][0] * c4 + m.columns[0][1] * c2 - m.columns[0][3] * c0) * invdet;
	r.columns[2][2] = (m.columns[3][0] * s4 - m.columns[3][1] * s2 + m.columns[3][3] * s0) * invdet;
	r.columns[2][3] = (-m.columns[2][0] * s4 + m.columns[2][1] * s2 - m.columns[2][3] * s0) * invdet;

	r.columns[3][0] = (-m.columns[1][0] * c3 + m.columns[1][1] * c1 - m.columns[1][2] * c0) * invdet;
	r.columns[3][1] = (m.columns[0][0] * c3 - m.columns[0][1] * c1 + m.columns[0][2] * c0) * invdet;
	r.columns[3][2] = (-m.columns[3][0] * s3 + m.columns[3][1] * s1 - m.columns[3][2] * s0) * invdet;
	r.columns[3][3] = (m.columns[2][0] * s3 - m.columns[2][1] * s1 + m.columns[2][2] * s0) * invdet;

	return r;
}


union Matrix4 LookAt(union Vector3 eye, union Vector3 target, union Vector3 up) {
    union Vector3 f = Normalize3(Sub3(target, eye));
    union Vector3 u = Normalize3(up);
    union Vector3 s = Normalize3( Cross3(f, u));
    u = Cross3(s, f);

    union Matrix4 m = Matrix4(1.0f);

    m.columns[0][0] = s.x;
    m.columns[1][0] = s.y;
    m.columns[2][0] = s.z;

    m.columns[0][1] = u.x;
    m.columns[1][1] = u.y;
    m.columns[2][1] = u.z;
    
    m.columns[0][2] = -f.x;
    m.columns[1][2] = -f.y;
    m.columns[2][2] = -f.z;

    union Matrix4 t = Translation(Negate3(eye));

    return MulM4(m, t);
}


union Matrix4 MulM4(union Matrix4 l, union Matrix4 r) {
    union Matrix4 m = Matrix4(0.0f);

    for (int column=0; column<4; ++column) {
        for (int row=0; row<4; ++row) {
            m.columns[column][row] =
                l.columns[0][row] * r.columns[column][0] +
                l.columns[1][row] * r.columns[column][1] +
                l.columns[2][row] * r.columns[column][2] +
                l.columns[3][row] * r.columns[column][3];
        }
    }

    return m;
}


union Matrix4 Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    union Matrix4 m = Matrix4(0.0f);

    m.columns[0][0] = 2.0f / (right - left);

    m.columns[1][1] = 2.0f / (top - bottom);

    m.columns[2][2] = -2.0f / (far - near);

    m.columns[3][0] = -(right + left) / (right - left);
    m.columns[3][1] = -(top + bottom) / (top - bottom);
    m.columns[3][2] = -(far + near) / (far - near);
    m.columns[3][3] = 1.0f;
    
    return m;
}


union Matrix4 Perspective(f32 fov, f32 aspect, f32 near, f32 far) {
    union Matrix4 m = Matrix4(1.0f);

    f32 f = tanf((fov / 2) * RADIANS);

    m.floats[0] = 1.0f / f;

    m.floats[5] = aspect / f;

    m.floats[10] = (near + far) / (near - far);
    m.floats[11] = -1.0f;

    m.floats[14] = (2.0f * near * far) / (near - far);
    m.floats[15] = 0.0f;

    return m;
}


union Matrix4 Rotation(union Quaternion q) {
    q = NormalizeQ(q);

    union Matrix4 l = { .floats={ q.w,  q.z, -q.y,  q.x,
                            -q.z,  q.w,  q.x,  q.y,
                            q.y, -q.x,  q.w,  q.z,
                            -q.x, -q.y, -q.z,  q.w }};

    union Matrix4 r = { .floats={ q.w,  q.z, -q.y, -q.x,
                            -q.z,  q.w,  q.x, -q.y,
                            q.y, -q.x,  q.w, -q.z,
                            q.x,  q.y,  q.z,  q.w }};

    return MulM4(l, r);
}


union Matrix4 Scale(union Vector3 v) {
    union Matrix4 m = Matrix4(1.0f);
    m.columns[0][0] = v.x;
    m.columns[1][1] = v.y;
    m.columns[2][2] = v.z;
    return m;
}


union Matrix4 Transformation(union Vector3 translation, union Quaternion rotation, union Vector3 scale) {
    return MulM4(Translation(translation),
                  MulM4(Rotation(rotation),
                         Scale(scale)));
}


union Vector4 Transform4(union Matrix4 l, union Vector4 r) {
    union Vector4 v = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

    for (int row=0; row<4; ++row) {
        f32 sum = 0.0f;

        for (int column=0; column<4; ++column) {
            sum += l.columns[column][row] * r.floats[column];
        }

        v.floats[row] = sum;
    }

    return v;
}


union Matrix4 Translation(union Vector3 v) {
    union Matrix4 m = Matrix4(1.0f);
    m.columns[3][0] = v.x;
    m.columns[3][1] = v.y;
    m.columns[3][2] = v.z;
    return m;
}


union IRect IRect(i32 x, i32 y, i32 width, i32 height) {
    return (union IRect) { .x=x, .y=y, .width=width, .height=height };
}


union Rect Rect(f32 x, f32 y, f32 width, f32 height) {
    return (union Rect) { .x=x, .y=y, .width=width, .height=height };
}


static union Vector2 to_barycentric(union Vector2 p, union Triangle2 t) {
    union Vector2 ab, ac, ap;
    ab = Sub2(t.b, t.a);
    ac = Sub2(t.c, t.a);
    ap = Sub2(p, t.a);

    float d00 = Dot2(ab, ab);
    float d01 = Dot2(ab, ac);
    float d11 = Dot2(ac, ac);
    float d20 = Dot2(ap, ab);
    float d21 = Dot2(ap, ac);

    float d = 1.0 / (d00 * d11 - d01 * d01);

    float u = (d11 * d20 - d01 * d21) * d;
    float v = (d00 * d21 - d01 * d20) * d;
    /* float w = 1.0f - u - v; */

    return Vector2(u, v);
}


static union Vector2 from_barycentric(union Vector2 p, union Triangle2 t) {
    union Vector2 ab, ac;
    ab = Sub2(t.b, t.a);
    ac = Sub2(t.c, t.a);
    return Add2(Add2(Scale2(ab, p.u), Scale2(ac, p.v)), t.a);
}


int InsideTriangle2(union Vector2 p, union Triangle2 t) {
    union Vector2 uv = to_barycentric(p, t);
    return (0 <= uv.u && 0 <= uv.v && uv.u + uv.v <= 1);
}


const u8 hash[] = {
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
    57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
    74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
    60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
    65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
    52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
    81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,

    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
    57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
    74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
    60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
    65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
    52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
    81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180
};


const u8 hash_mask = 255;


static float smooth(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}


f32 Value1(f32 point) {
    i32 i = (i32)floorf (point);
    i &= hash_mask;
    return hash[i] * (1.0f / (f32)hash_mask);
}


f32 Value2(union Vector2 point) {
    i32 ix0 = (i32)floorf (point.x);
    i32 iy0 = (i32)floorf (point.y);

    f32 tx = point.x - ix0;
    f32 ty = point.y - iy0;

    ix0 &= hash_mask;
    iy0 &= hash_mask;

    i32 ix1 = ix0 + 1;
    i32 iy1 = iy0 + 1;

    u8 h0 = hash[ix0];
    u8 h1 = hash[ix1];
    u8 h00 = hash[h0 + iy0];
    u8 h10 = hash[h1 + iy0];
    u8 h01 = hash[h0 + iy1];
    u8 h11 = hash[h1 + iy1];

    tx = smooth(tx);
    ty = smooth(ty);
    return lerpf(lerpf(h00, h10, tx),
                 lerpf(h01, h11, tx),
                 ty) * (1.0f / hash_mask);
}


static union Vector2 voroni_neighbor(int x, int y) {
    union Vector2 neighbor = { .x=Value1(x + 7204 * y),
                               .y=Value1(y - 1242 * x) /* To prevent identical coordinates */ };
    return neighbor;
}


f32 Voroni2(union Vector2 point, f32 scale) {
    // Get cell coordinates
    int x = floorf(point.x / scale);
    int y = floorf(point.y / scale);
    
    // Transform point into cell coordinates
    point.x = fmodf(point.x, scale) / scale;
    point.y = fmodf(point.y, scale) / scale;

    // Find nearest neighbor
    f32 nearest = 1.0f;
    for (int oy=-1; oy<=1; ++oy) {
    	for (int ox=-1; ox<=1; ++ox) {
            // Get the current neighbor
    	    union Vector2 neighbor = voroni_neighbor(x + ox, y + oy);
            neighbor = Add2(neighbor, Vector2 (ox, oy));

            // Find the distance between point and neighbor
    	    union Vector2 difference = Sub2(point, neighbor);
    	    f32 distance = MagnitudeSquared2(difference);

            // Compare shortest distances
    	    nearest = fminf(nearest, distance);
    	}
    }

    nearest = sqrtf(nearest);
    
    return nearest;
}


f32 FastVoroni2(union Vector2 point, f32 scale) {
    // Get cell coordinates
    int x = floorf(point.x / scale);
    int y = floorf(point.y / scale);
    
    // Transform point into cell coordinates
    point.x = fmodf(point.x, scale) / scale;
    point.y = fmodf(point.y, scale) / scale;

    // Find nearest neighbor
    f32 nearest = 1.0f;
    for (int oy=0; oy<=1; ++oy) {
    	for (int ox=0; ox<=1; ++ox) {
            // Get the current neighbor
    	    union Vector2 neighbor = voroni_neighbor(x + ox, y + oy);
            neighbor = Add2(neighbor, Vector2(ox, oy));

            // Find the distance between point and neighbor
    	    union Vector2 difference = Sub2(point, neighbor);
    	    f32 distance = MagnitudeSquared2(difference);

            // Compare shortest distances
    	    nearest = fminf(nearest, distance);
    	}
    }

    nearest = sqrtf(nearest);
    
    return nearest;
}
