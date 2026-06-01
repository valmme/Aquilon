#ifndef AQUILON_VMATH_H
#define AQUILON_VMATH_H

#include <cmath>
#include <SDL3/SDL.h>

struct vec2 {
    float x, y;
};

struct vec3 {
    float x, y, z;
};

struct vec4 {
    float x, y, z, w;
};

static inline vec2 vec2add(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
static inline vec2 vec2sub(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
static inline vec2 vec2mul(vec2 a, float v) { return {a.x * v, a.y * v}; }
static inline vec2 vec2div(vec2 a, float v) { return {a.x / v, a.y / v}; }

static inline float vec2len(vec2 a) { return std::sqrt(a.x * a.x + a.y * a.y); }

static inline vec2 vec2norm(vec2 a) {
    float len = vec2len(a);
    if (len == 0.0f) return {0, 0};
    return {a.x / len, a.y / len};
}

static inline float vec2distance(vec2 a, vec2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

static inline vec3 vec3add(vec3 a, vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline vec3 vec3sub(vec3 a, vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline vec3 vec3mul(vec3 a, float v) { return {a.x * v, a.y * v, a.z * v}; }

static inline float vec3len(vec3 a) {
    return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

static inline vec3 vec3_normalize(vec3 a) {
    float len = vec3len(a);
    if (len == 0.0f) return {0, 0, 0};
    return {a.x / len, a.y / len, a.z / len};
}   

static inline float vec3distance(vec3 a, vec3 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

static inline bool point_in_rec(float px, float py, const SDL_FRect& r) {
    return px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h;
}

static inline bool vec_in_rec(vec2 p, const SDL_FRect& r) {
    return point_in_rec(p.x, p.y, r);
}

#endif // AQUILON_VMATH_H
