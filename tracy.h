#ifndef TRACY_TRACER_H
#define TRACY_TRACER_H

#ifdef __cplusplus
#define extern "C" {
#endif

/****************
Tracy path tracer
****************/

#include <libfract.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

typedef struct Ray {
    vec3 orig;
    vec3 dir;
} Ray;

typedef struct Hit {
    vec3 pos;
    vec3 normal;
    float t;
} Hit;

typedef struct Sphere {   
    vec3 center;
    float radius;
} Sphere;

typedef enum {
    Lambert, 
    Metal,
    Dielectric
} MatEnum;

typedef struct Material {
    MatEnum type;
    vec3 albedo;
    vec3 emissive;
    float roughness;
    float ri;
} Material;

typedef struct Camera {
    vec3 origin;
    vec3 lowerLeftCorner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    float lensRadius;
} Camera;

#define AssertUnit(v)                               \
do {                                                \
    if (!(fabsf(vec3_sqmag(v) - 1.0f) < 0.01f)) {   \
        printf("%d, %s - ", __LINE__, __func__);    \
        vec3_print(v);                              \
    }                                               \
} while (0)

/* ... */

vec3 vec3_reflect(vec3 v, vec3 n);
bool vec3_refract(vec3 v, vec3 n, float nint, vec3* outRefracted);

vec3 random_in_disk();
vec3 random_in_sphere();

Ray ray_new(vec3 orig, vec3 dir);
vec3 ray_at(Ray* ray, float t);

bool sphere_hit(Ray* ray, Sphere* sphere, float tMin, float tMax, Hit* outHit);

Camera camera_new(vec3 lookFrom, vec3 lookAt, vec3 vup, float vfov, float aspect, float aperture, float focusDist);
Ray camera_ray(Camera* cam, float s, float t);

#ifdef __cplusplus
}
#endif
#endif