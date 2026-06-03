#ifndef AQUILON_VMATH_H
#define AQUILON_VMATH_H

#include <cmath>
#include <SDL3/SDL.h>

struct vec2 {
    float x, y;
};

using Vec2 = vec2;

struct vec3 {
    float x, y, z;
};

struct vec4 {
    float x, y, z, w;
};

static inline vec2 Vec2Add(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
static inline vec2 Vec2Sub(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
static inline vec2 Vec2Mul(vec2 a, float v) { return {a.x * v, a.y * v}; }
static inline vec2 Vec2Div(vec2 a, float v) { return {a.x / v, a.y / v}; }

static inline float Vec2Len(vec2 a) { return std::sqrt(a.x * a.x + a.y * a.y); }

static inline vec2 Vec2Norm(vec2 a) {
    float len = Vec2Len(a);
    if (len == 0.0f) return {0, 0};
    return {a.x / len, a.y / len};
}

static inline float Vec2Distance(vec2 a, vec2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

static inline vec2 Vec2Zero() { return {0, 0}; }
static inline vec3 Vec3Zero() { return {0, 0, 0}; }

static inline vec3 Vec3Add(vec3 a, vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline vec3 Vec3Sub(vec3 a, vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline vec3 Vec3Mul(vec3 a, float v) { return {a.x * v, a.y * v, a.z * v}; }

static inline float Vec3Len(vec3 a) {
    return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

static inline vec3 Vec3Normalize(vec3 a) {
    float len = Vec3Len(a);
    if (len == 0.0f) return {0, 0, 0};
    return {a.x / len, a.y / len, a.z / len};
}   

static inline float Vec3Distance(vec3 a, vec3 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// rectangles
static inline bool PointInRec(float px, float py, const SDL_FRect& r) {
    return px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h;
}

static inline bool VecInRec(vec2 p, const SDL_FRect& r) {
    return PointInRec(p.x, p.y, r);
}

#endif // AQUILON_VMATH_H
