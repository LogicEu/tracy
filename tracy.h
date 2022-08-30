#ifndef TRACY_TRACER_H
#define TRACY_TRACER_H

#ifdef __cplusplus
#define extern "C" {
#endif

/****************
tracy path tracer
*****************
@Eugenio Arteaga
****************/

#include <stdint.h>
#include <mass/mass.h>
#include <photon/photon.h>
#include <imgtool/imgtool.h>

/* tracy's configurations */

#define TRACY_PERF
#define TRACY_MAX_DEPTH 4
#define TRACY_MIN_DIST 0.001f
#define TRACY_MAX_DIST 1.0e7f
#define TRACY_OCTREE_LIMIT 8

/* tracy's structs */

typedef struct Material {
    enum MatType {
        Invisible,
        Lambert, 
        Metal,
        Dielectric
    } type;
    vec3 albedo;
    vec3 emissive;
    float roughness;
    float ri;
} Material;

typedef struct Cam3D {
    struct CamParam3D {
        vec3 lowerLeftCorner;
        vec3 horizontal;
        vec3 vertical;
        vec3 u, v, w;
    } params;
    vec3 lookFrom;
    vec3 lookAt;
    vec3 up;
    float fov;
    float aspect;
    float aperture;
    float focusDist;
} Cam3D;

typedef struct Oct3D {
    Box3D box;
    struct Oct3D* children;
    array_t triangles;
} Oct3D;

typedef struct Model3D {
    array_t triangles;
    Oct3D octree;
} Model3D;

typedef struct Scene3D {
    Cam3D cam;
    array_t materials;
    array_t spheres;
    array_t sphere_materials;
    array_t triangles;
    array_t triangle_materials;
    array_t models;
    vec3 background_color;
} Scene3D;

typedef struct Render3D {
    uint8_t* buffer;
    uint32_t width;
    uint32_t height;
    uint32_t spp;
    uint32_t frames;
    uint32_t threads;
    uint32_t timer;
} Render3D;

/* tracy's API */

double time_clock();

Render3D render3D_new(const uint32_t width, const uint32_t height, const uint32_t spp);
bmp_t render3D_bmp(const Render3D* render, const Scene3D* scene);
void render3D_render(const Render3D* render, const Scene3D* scene);
void render3D_set(Render3D* render);
void render3D_free(Render3D* render);

Model3D* model3D_load(const char* filename);
void model3D_free(Model3D* model);
void model3D_move(const Model3D* model, const vec3 trans);
void model3D_scale(const Model3D* model, const float scale);
void model3D_scale3D(const Model3D* model, const vec3 scale);

Scene3D* scene3D_load(const char* filename, const float aspect);
void scene3D_write(const char* filename, const Scene3D* scene);
bool scene3D_hit(const Scene3D* scene, const Ray3D* ray, Hit3D* outHit, size_t* outID);
void scene3D_free(Scene3D* free);

Cam3D cam3D_new(const vec3 lookFrom, const vec3 lookAt, const vec3 up, const float fov, const float aspect, const float aperture, const float focusDist);
Ray3D cam3D_ray(const Cam3D* cam, const float s, const float p);
void cam3D_update(Cam3D* cam);

vec3 ray3D_trace(const Scene3D* scene, const Ray3D* ray, const uint32_t depth);

Oct3D oct3D_create(const Box3D box);
Oct3D oct3D_from_mesh(const Tri3D* triangles, const size_t count);
bool oct3D_hit(const Oct3D* oct, const Ray3D* ray, Hit3D* hit, float closest);
void oct3D_free(Oct3D* oct);

int tracy_error(const char* str);
int tracy_version(void);
int tracy_help(const int runtime);
int tracy_log_render3D(const Render3D* render);
int tracy_log_time(const float time);

#ifdef __cplusplus
}
#endif
#endif
