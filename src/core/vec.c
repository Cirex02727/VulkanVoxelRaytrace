#include "vec.h"

#include <string.h>
#include <float.h>
#include <math.h>

float deg_to_rad(float a)
{
    return a * PI / 180.0f;
}

float clamp(float v, float min, float max)
{
    const float t = v < min ? min : v;
    return t > max ? max : t;
}

// ##### Vec2 #####

void vec2_add(Vec2* v0, Vec2* v1, Vec2* out)
{
    out->x = v0->x + v1->x;
    out->y = v0->y + v1->y;
}

bool vec2_is_zero(Vec2* v)
{
    return v->x == 0 && v->y == 0;
}

// ##### Vec2 #####

// ##### Vec3 #####

void vec3_add(Vec3* v0, Vec3* v1, Vec3* out)
{
    out->x = v0->x + v1->x;
    out->y = v0->y + v1->y;
    out->z = v0->z + v1->z;
}

void vec3_addn(Vec3* v, float n, Vec3* out)
{
    out->x = v->x + n;
    out->y = v->y + n;
    out->z = v->z + n;
}

void vec3_sub(Vec3* v0, Vec3* v1, Vec3* out)
{
    out->x = v0->x - v1->x;
    out->y = v0->y - v1->y;
    out->z = v0->z - v1->z;
}

void vec3_subn(Vec3* v, float n, Vec3* out)
{
    out->x = v->x - n;
    out->y = v->y - n;
    out->z = v->z - n;
}

void vec3_isub(Vec3* v0, IVec3* v1, Vec3* out)
{
    out->x = v0->x - v1->x;
    out->y = v0->y - v1->y;
    out->z = v0->z - v1->z;
}

void vec3_mul(Vec3* v0, Vec3* v1, Vec3* out)
{
    out->x = v0->x * v1->x;
    out->y = v0->y * v1->y;
    out->z = v0->z * v1->z;
}

void vec3_div(Vec3* v0, Vec3* v1, Vec3* out)
{
    out->x = v0->x / v1->x;
    out->y = v0->y / v1->y;
    out->z = v0->z / v1->z;
}

void vec3_scale(Vec3* v, float s, Vec3* out)
{
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}

void vec3_madn(Vec3* v0, float n, Vec3* v1, Vec3* out)
{
    out->x = v0->x * n + v1->x;
    out->y = v0->y * n + v1->y;
    out->z = v0->z * n + v1->z;
}

float vec3_dot(Vec3* v0, Vec3* v1)
{
    return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

void vec3_cross(Vec3* v0, Vec3* v1, Vec3* out)
{
    out->x = v0->y * v1->z - v0->z * v1->y;
    out->y = v0->z * v1->x - v0->x * v1->z;
    out->z = v0->x * v1->y - v0->y * v1->x;
}

float vec3_norm2(Vec3* v)
{
    return vec3_dot(v, v);
}

float vec3_norm(Vec3* v)
{
    return sqrtf(vec3_norm2(v));
}

void vec3_normalize(Vec3* v, Vec3* out)
{
    float norm = vec3_norm(v);
    
    if (norm < FLT_EPSILON) {
        *v = (Vec3) { 0.0f, 0.0f, 0.0f };
        return;
    }

    vec3_scale(v, 1.0f / norm, out);
}

void vec3_inv(Vec3* v, Vec3* out)
{
    out->x = 1.0f / v->x;
    out->y = 1.0f / v->y;
    out->z = 1.0f / v->z;
}

void vec3_ceil(Vec3* v, Vec3* out)
{
    out->x = ceilf(v->x);
    out->y = ceilf(v->y);
    out->z = ceilf(v->z);
}

void vec3_floor(Vec3* v, Vec3* out)
{
    out->x = floorf(v->x);
    out->y = floorf(v->y);
    out->z = floorf(v->z);
}

void vec3_floori(Vec3* v, IVec3* out)
{
    out->x = (int) floorf(v->x);
    out->y = (int) floorf(v->y);
    out->z = (int) floorf(v->z);
}

void vec3_sign(Vec3* v, Vec3* out)
{
    out->x = (v->x >= 0) ? 1 : -1;
    out->y = (v->y >= 0) ? 1 : -1;
    out->z = (v->z >= 0) ? 1 : -1;
}

void vec3_abs(Vec3* v, Vec3* out)
{
    out->x = fabsf(v->x);
    out->y = fabsf(v->y);
    out->z = fabsf(v->z);
}

void vec3_modn(Vec3* v, float n, Vec3* out)
{
    out->x = fmodf(v->x, n);
    out->y = fmodf(v->y, n);
    out->z = fmodf(v->z, n);
}

void vec3_crossn(Vec3* v0, Vec3* v1, Vec3* out)
{
    vec3_cross(v0, v1, out);
    vec3_normalize(out, out);
}

float vec3_distance2(Vec3* v0, Vec3* v1)
{
    Vec3 diff;
    vec3_sub(v0, v1, &diff);
    return vec3_dot(&diff, &diff);
}

void vec3_lerp(Vec3* v0, Vec3* v1, float t, Vec3* out)
{
    out->x = v0->x + t * (v1->x - v0->x);
    out->y = v0->y + t * (v1->y - v0->y);
    out->z = v0->z + t * (v1->z - v0->z);
}

bool vec3_is_zero(Vec3* v)
{
    return v->x == 0 && v->y == 0 && v->z == 0;
}

// ##### Vec3 #####

// ##### IVec3 #####

void ivec3_vec3(Vec3* v, IVec3* out)
{
    out->x = (int) v->x;
    out->y = (int) v->y;
    out->z = (int) v->z;
}

void ivec3_subof(IVec3* v0, IVec3* v1, Vec3* out)
{
    out->x = v0->x - v1->x;
    out->y = v0->y - v1->y;
    out->z = v0->z - v1->z;
}

void ivec3_addf(IVec3* v0, Vec3* v1, Vec3* out)
{
    out->x = v0->x + v1->x;
    out->y = v0->y + v1->y;
    out->z = v0->z + v1->z;
}

void ivec3_lerp(IVec3* v0, IVec3* v1, float t, Vec3* out)
{
    out->x = v0->x + t * (v1->x - v0->x);
    out->y = v0->y + t * (v1->y - v0->y);
    out->z = v0->z + t * (v1->z - v0->z);
}

// ##### IVec3 #####

// ##### Vec4 #####

void vec4_mad(Vec4* v, float n, Vec4* out)
{
    out->x += v->x * n;
    out->y += v->y * n;
    out->z += v->z * n;
    out->w += v->w * n;
}

// ##### Vec4 #####

// ##### Mat4 #####

void mat4_identity(Mat4* m)
{
    *m = (Mat4) {
        .r0 = { 1.0f, 0.0f, 0.0f, 0.0f },
        .r1 = { 0.0f, 1.0f, 0.0f, 0.0f },
        .r2 = { 0.0f, 0.0f, 1.0f, 0.0f },
        .r3 = { 0.0f, 0.0f, 0.0f, 1.0f },
    };
}

void mat4_zero(Mat4* m)
{
    memset(m, 0, sizeof(Mat4));
}

void mat4_mul(Mat4* m0, Mat4* m1, Mat4* out)
{
    float a00 = m0->r0.x, a01 = m0->r0.y, a02 = m0->r0.z, a03 = m0->r0.w,
          a10 = m0->r1.x, a11 = m0->r1.y, a12 = m0->r1.z, a13 = m0->r1.w,
          a20 = m0->r2.x, a21 = m0->r2.y, a22 = m0->r2.z, a23 = m0->r2.w,
          a30 = m0->r3.x, a31 = m0->r3.y, a32 = m0->r3.z, a33 = m0->r3.w,

          b00 = m1->r0.x, b01 = m1->r0.y, b02 = m1->r0.z, b03 = m1->r0.w,
          b10 = m1->r1.x, b11 = m1->r1.y, b12 = m1->r1.z, b13 = m1->r1.w,
          b20 = m1->r2.x, b21 = m1->r2.y, b22 = m1->r2.z, b23 = m1->r2.w,
          b30 = m1->r3.x, b31 = m1->r3.y, b32 = m1->r3.z, b33 = m1->r3.w;

    out->r0.x = a00 * b00 + a10 * b01 + a20 * b02 + a30 * b03;
    out->r0.y = a01 * b00 + a11 * b01 + a21 * b02 + a31 * b03;
    out->r0.z = a02 * b00 + a12 * b01 + a22 * b02 + a32 * b03;
    out->r0.w = a03 * b00 + a13 * b01 + a23 * b02 + a33 * b03;
    out->r1.x = a00 * b10 + a10 * b11 + a20 * b12 + a30 * b13;
    out->r1.y = a01 * b10 + a11 * b11 + a21 * b12 + a31 * b13;
    out->r1.z = a02 * b10 + a12 * b11 + a22 * b12 + a32 * b13;
    out->r1.w = a03 * b10 + a13 * b11 + a23 * b12 + a33 * b13;
    out->r2.x = a00 * b20 + a10 * b21 + a20 * b22 + a30 * b23;
    out->r2.y = a01 * b20 + a11 * b21 + a21 * b22 + a31 * b23;
    out->r2.z = a02 * b20 + a12 * b21 + a22 * b22 + a32 * b23;
    out->r2.w = a03 * b20 + a13 * b21 + a23 * b22 + a33 * b23;
    out->r3.x = a00 * b30 + a10 * b31 + a20 * b32 + a30 * b33;
    out->r3.y = a01 * b30 + a11 * b31 + a21 * b32 + a31 * b33;
    out->r3.z = a02 * b30 + a12 * b31 + a22 * b32 + a32 * b33;
    out->r3.w = a03 * b30 + a13 * b31 + a23 * b32 + a33 * b33;
}

void mat4_scale(Mat4* m, Vec3* v)
{
    m->r0.x *= v->x;
    m->r1.y *= v->y;
    m->r2.z *= v->z;
}

void mat4_scale_p(Mat4* m, float s)
{
    m->r0.x *= s; m->r0.y *= s; m->r0.z *= s; m->r0.w *= s;
    m->r1.x *= s; m->r1.y *= s; m->r1.z *= s; m->r1.w *= s;
    m->r2.x *= s; m->r2.y *= s; m->r2.z *= s; m->r2.w *= s;
    m->r3.x *= s; m->r3.y *= s; m->r3.z *= s; m->r3.w *= s;
}

void mat4_perspective(float fovY, float aspect, float nearZ, float farZ, Mat4* out)
{
    mat4_zero(out);
    
    float f  = 1.0f / tanf(fovY * 0.5f);
    float fn = 1.0f / (nearZ - farZ);

    out->r0.x =  f / aspect;
    out->r1.y =  f;
    out->r2.z =  (nearZ + farZ) * fn;
    out->r2.w = -1.0f;
    out->r3.z =  2.0f * nearZ * farZ * fn;
}

void mat4_ortho(float left, float right, float bottom, float top, float zNear, float zFar, Mat4* out)
{
    mat4_zero(out);

    float rl =  1.0f / (right - left);
    float tb =  1.0f / (top   - bottom);
    float fn = -1.0f / (zFar  - zNear);

    out->r0.x =  2.0f * rl;
    out->r1.y =  2.0f * tb;
    out->r2.z =  2.0f * fn;
    out->r3.x = -(right + left)   * rl;
    out->r3.y = -(top   + bottom) * tb;
    out->r3.z =  (zFar  + zNear)  * fn;
    out->r3.w =  1.0f;
}

void mat4_lookat(Vec3* eye, Vec3* center, Vec3* up, Mat4* out)
{
    Vec3 f;
    vec3_sub(center, eye, &f);
    vec3_normalize(&f, &f);
    
    Vec3 s, u;
    vec3_crossn(&f, up, &s);
    vec3_cross(&s, &f, &u);

    out->r0.x  =  s.x;
    out->r0.y  =  u.x;
    out->r0.z  = -f.x;
    out->r0.w  =  0.0f;
    out->r1.x  =  s.y;
    out->r1.y  =  u.y;
    out->r1.z  = -f.y;
    out->r1.w  =  0.0f;
    out->r2.x =  s.z;
    out->r2.y =  u.z;
    out->r2.z = -f.z;
    out->r2.w =  0.0f;
    out->r3.x = -vec3_dot(&s, eye);
    out->r3.y = -vec3_dot(&u, eye);
    out->r3.z =  vec3_dot(&f, eye);
    out->r3.w =  1.0f;
}

void mat4_inv(Mat4* mat, Mat4* out)
{
    float t[6];
    float det;
    float a = mat->r0.x, b = mat->r0.y, c = mat->r0.z, d = mat->r0.w,
          e = mat->r1.x, f = mat->r1.y, g = mat->r1.z, h = mat->r1.w,
          i = mat->r2.x, j = mat->r2.y, k = mat->r2.z, l = mat->r2.w,
          m = mat->r3.x, n = mat->r3.y, o = mat->r3.z, p = mat->r3.w;

    t[0] = k * p - o * l; t[1] = j * p - n * l; t[2] = j * o - n * k;
    t[3] = i * p - m * l; t[4] = i * o - m * k; t[5] = i * n - m * j;

    out->r0.x =  f * t[0] - g * t[1] + h * t[2];
    out->r1.x =-(e * t[0] - g * t[3] + h * t[4]);
    out->r2.x =  e * t[1] - f * t[3] + h * t[5];
    out->r3.x =-(e * t[2] - f * t[4] + g * t[5]);

    out->r0.y =-(b * t[0] - c * t[1] + d * t[2]);
    out->r1.y =  a * t[0] - c * t[3] + d * t[4];
    out->r2.y =-(a * t[1] - b * t[3] + d * t[5]);
    out->r3.y =  a * t[2] - b * t[4] + c * t[5];

    t[0] = g * p - o * h; t[1] = f * p - n * h; t[2] = f * o - n * g;
    t[3] = e * p - m * h; t[4] = e * o - m * g; t[5] = e * n - m * f;

    out->r0.z =  b * t[0] - c * t[1] + d * t[2];
    out->r1.z =-(a * t[0] - c * t[3] + d * t[4]);
    out->r2.z =  a * t[1] - b * t[3] + d * t[5];
    out->r3.z =-(a * t[2] - b * t[4] + c * t[5]);

    t[0] = g * l - k * h; t[1] = f * l - j * h; t[2] = f * k - j * g;
    t[3] = e * l - i * h; t[4] = e * k - i * g; t[5] = e * j - i * f;

    out->r0.w =-(b * t[0] - c * t[1] + d * t[2]);
    out->r1.w =  a * t[0] - c * t[3] + d * t[4];
    out->r2.w =-(a * t[1] - b * t[3] + d * t[5]);
    out->r3.w =  a * t[2] - b * t[4] + c * t[5];

    det = 1.0f / (a * out->r0.x + b * out->r1.x
    + c * out->r2.x + d * out->r3.x);

    mat4_scale_p(out, det);
}

void mat4_translate(Mat4* m, Vec3* v, Mat4* out)
{
    vec4_mad(&m->r0, v->x, &out->r3);
    vec4_mad(&m->r1, v->y, &out->r3);
    vec4_mad(&m->r2, v->z, &out->r3);
}

void mat4_transpose(Mat4* m, Mat4* out)
{
    out->r0.x = m->r0.x; out->r1.x = m->r0.y;
    out->r0.y = m->r1.x; out->r1.y = m->r1.y;
    out->r0.z = m->r2.x; out->r1.z = m->r2.y;
    out->r0.w = m->r3.x; out->r1.w = m->r3.y;
    out->r2.x = m->r0.z; out->r3.x = m->r0.w;
    out->r2.y = m->r1.z; out->r3.y = m->r1.w;
    out->r2.z = m->r2.z; out->r3.z = m->r2.w;
    out->r2.w = m->r3.z; out->r3.w = m->r3.w;
}

// ##### Mat4 #####
