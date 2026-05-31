#pragma once
#include <math.h>

typedef struct {
    float x, y;
} vec2;

typedef struct {
    float x, y, z;
} vec3;

typedef struct {
    float x, y, z, w;
} vec4;

// vec2
static inline vec2  vec2add(vec2 a, vec2 b)  { return (vec2){a.x + b.x, a.y + b.y}; }
static inline vec2  vec2sub(vec2 a, vec2 b)  { return (vec2){a.x - b.x, a.y - b.y}; }
static inline vec2  vec2mul(vec2 a, float v) { return (vec2){a.x * v  , a.y * v  }; }
static inline vec2  vec2div(vec2 a, float v) { return (vec2){a.x / v  , a.y / v  }; }
static inline float vec2len(vec2 a)          { return  sqrtf(a.x*a.x  + a.y*a.y);   }

static inline vec2 vec2norm(vec2 a) {
    float len = vec2len(a);
    if (len == 0.0f) return (vec2){0, 0};
    return (vec2){a.x / len, a.y / len};
}

static inline float vec2distance(vec2 a, vec2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

// vec2
static inline vec3 vec3add(vec3 a, vec3 b)  { return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};  }
static inline vec3 vec3sub(vec3 a, vec3 b)  { return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};  }
static inline vec3 vec3mul(vec3 a, float v) { return (vec3){a.x * v , a.y * v  , a.z * v};     }
static inline float vec3len(vec3 a)      { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }

static inline vec3 vec3_normalize(vec3 a) {
    float len = vec3len(a);
    if (len == 0.0f) return (vec3){0, 0, 0};
    return (vec3){a.x / len, a.y / len, a.z / len};
}

static inline float vec3distance(vec3 a, vec3 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}