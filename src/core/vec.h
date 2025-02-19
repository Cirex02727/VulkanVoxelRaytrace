#ifndef VEC_H_
#define VEC_H_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define PI 3.14159265358979323846264338327950288

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    uint32_t x, y;
} UIVec2;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    int x, y, z;
} IVec3;

typedef struct {
    float x, y, z, w;
} Vec4;

typedef struct {
    Vec4 r0, r1, r2, r3;
} Mat4;

float deg_to_rad(float a);

float clamp(float v, float min, float max);

// ##### Vec2 #####

void vec2_add(Vec2* v0, Vec2* v1, Vec2* out);

bool vec2_is_zero(Vec2* v);

// ##### Vec2 #####

// ##### Vec3 #####

void vec3_add(Vec3* v0, Vec3* v1, Vec3* out);

void vec3_addn(Vec3* v, float n, Vec3* out);

void vec3_sub(Vec3* v0, Vec3* v1, Vec3* out);

void vec3_subn(Vec3* v, float n, Vec3* out);

void vec3_isub(Vec3* v0, IVec3* v1, Vec3* out);

void vec3_mul(Vec3* v0, Vec3* v1, Vec3* out);

void vec3_div(Vec3* v0, Vec3* v1, Vec3* out);

void vec3_scale(Vec3* v, float s, Vec3* out);

void vec3_madn(Vec3* v0, float n, Vec3* v1, Vec3* out);

float vec3_dot(Vec3* v0, Vec3* v1);

void vec3_cross(Vec3* v0, Vec3* v1, Vec3* out);

float vec3_norm2(Vec3* v);

float vec3_norm(Vec3* v);

void vec3_normalize(Vec3* v, Vec3* out);

void vec3_inv(Vec3* v, Vec3* out);

void vec3_ceil(Vec3* v, Vec3* out);

void vec3_floor(Vec3* v, Vec3* out);

void vec3_floori(Vec3* v, IVec3* out);

void vec3_sign(Vec3* v, Vec3* out);

void vec3_abs(Vec3* v, Vec3* out);

void vec3_modn(Vec3* v, float n, Vec3* out);

void vec3_crossn(Vec3* v0, Vec3* v1, Vec3* out);

float vec3_distance2(Vec3* v0, Vec3* v1);

void vec3_lerp(Vec3* v0, Vec3* v1, float t, Vec3* out);

bool vec3_is_zero(Vec3* v);

// ##### Vec3 #####

// ##### IVec3 #####

void ivec3_vec3(Vec3* v, IVec3* out);

void ivec3_subof(IVec3* v0, IVec3* v1, Vec3* out);

void ivec3_addf(IVec3* v0, Vec3* v1, Vec3* out);

void ivec3_lerp(IVec3* v0, IVec3* v1, float t, Vec3* out);

// ##### IVec3 #####

// ##### Vec4 #####

void vec4_mad(Vec4* v, float n, Vec4* out);

// ##### Vec4 #####

// ##### Mat4 #####

void mat4_identity(Mat4* m);

void mat4_zero(Mat4* m);

void mat4_mul(Mat4* m0, Mat4* m1, Mat4* out);

void mat4_scale(Mat4* m, Vec3* v);

void mat4_scale_p(Mat4* m, float s);

void mat4_perspective(float fovY, float aspect, float nearZ, float farZ, Mat4* out);

void mat4_ortho(float left, float right, float bottom, float top, float zNear, float zFar, Mat4* out);

void mat4_lookat(Vec3* eye, Vec3* center, Vec3* up, Mat4* out);

void mat4_inv(Mat4* mat, Mat4* out);

void mat4_translate(Mat4* m, Vec3* v, Mat4* out);

void mat4_transpose(Mat4* m, Mat4* out);

// ##### Mat4 #####

#endif // VEC_H_
