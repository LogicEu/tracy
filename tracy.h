#ifndef TRACY_TRACER_H
#define TRACY_TRACER_H

#ifdef __cplusplus
#define extern "C" {
#endif

/****************
tracy path tracer
****************/

#include <mass.h>
#include <photon.h>

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

typedef struct BoundingBox {
    vec3 min;
    vec3 max;
} BoundingBox;

typedef struct Cam3D {
    vec3 origin;
    vec3 lowerLeftCorner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    float lensRadius;
} Cam3D;

typedef struct JobData {
    int frameCount;
    int screenWidth, screenHeight;
    float* backbuffer;
    Cam3D* cam;
    volatile int rayCount;
} JobData;

/* ... */

#define PERF

extern BoundingBox boundingBox;

extern int samples_per_pixel;
extern float animate_smoothing;
extern bool light_sampling;
extern float fov;

extern JobData job;
extern Cam3D cam;
extern vec3 lookfrom;
extern vec3 lookat;
extern float distToFocus;
extern float aperture;

extern array_t triangles;
extern array_t trimaterials;
extern array_t spheres;
extern array_t sphmaterials;
extern array_t materials;

extern vec3 skyColor;
extern float skyMult;

/* ... */

double time_clock();
void scene_init();
void frame_render(int thread_count);

Cam3D camera_new(vec3 lookFrom, vec3 lookAt, vec3 vup, float vfov, float aspect, float aperture, float focusDist);
Ray3D camera_ray(const Cam3D* cam, float s, float t);

vec3 ray_trace(const Ray3D* ray, int depth, int* inoutRayCount);

#ifdef __cplusplus
}
#endif
#endif
