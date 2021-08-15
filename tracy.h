#ifndef TRACY_TRACER_H
#define TRACY_TRACER_H

#ifdef __cplusplus
#define extern "C" {
#endif

/****************
Tracy path tracer
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

extern int samples_per_pixel;
extern float animate_smoothing;
extern bool light_sampling;

extern JobData job;
extern Cam3D cam;
extern vec3 lookfrom;
extern vec3 lookat;
extern float distToFocus;
extern float aperture;

extern array_t* triangles;
extern array_t* spheres;
extern array_t* materials;

/* ... */

Cam3D camera_new(vec3 lookFrom, vec3 lookAt, vec3 vup, float vfov, float aspect, float aperture, float focusDist);
Ray3D camera_ray(const Cam3D* cam, float s, float t);
array_t* tracy_mesh_load(const char* path);

vec3 ray_trace(const Ray3D* restrict ray, int depth, int* inoutRayCount);

void frame_render();
void frame_render_threaded(int thread_count);

#ifdef __cplusplus
}
#endif
#endif
