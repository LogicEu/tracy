#include <tracy.h>
#include <imgtool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

const char* string_separator = "----------------------------------------------------------\n";

static int samples_per_pixel = 100;
static float animate_smoothing = 0.01f;

static array_t* triangles;

static Sphere spheres[] = {
    {{0, -100.5, -1}, 100},
    {{2, 1, -1}, 0.5f},
    {{0, 0, -1}, 0.5f},
    {{-2, 0, -1}, 0.5f},
    {{2, 0, 1}, 0.5f},
    {{0, -1, 1}, 0.5f},
    {{-2, 0, 1}, 0.5f},
    {{-1.f, 1.25, 3.f}, 0.5f},
    {{-2.0f, 2.5f, 4.f}, 0.5f}
};

static Material materials[] = {
    { Lambert, {0.8f, 0.8f, 0.8f}, {0, 0, 0}, 0, 0 },
    { Lambert, {0.2f, 0.4f, 0.8f}, {0.1, 0, 0}, 0, 0 },
    { Lambert, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0, 0 },
    { Metal, {1.0f, 1.0f, 1.0f}, {0, 0, 0}, 0.0, 0 },
    { Metal, {0.8f, 0.8f, 0.4f}, {0, 0, 0}, 0.8, 0 },
    { Metal, {0.2f, 0.2f, 0.8f}, {0, 0, 0}, 0.1f, 0 },
    { Metal, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0.8f, 0.8 },
    { Dielectric, {0.4f, 0.4f, 0.4f}, {0, 0, 0}, 0, 1.5f },
    { Lambert, {0.8f, 0.5f, 0.3f}, {20, 15, 5}, 0, 0 }
};

const int sphere_count = sizeof(spheres) / sizeof(spheres[0]);
const float kMinT = 0.001f;
const float kMaxT = 1.0e7f;
const int kMaxDepth = 10;

static float schlick(float cosine, float ri)
{
    float r0 = (1.0f - ri) / (1.0f + ri);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * powf(1.0 - cosine, 5.0);
}

static bool scene_hit(Ray* r, float tMin, float tMax, Hit* outHit, int* outID)
{
    Hit tmpHit;
    bool anything = false;
    float closest = tMax;
    for (int i = 0; i < sphere_count; ++i) {
        if (sphere_hit(r, &spheres[i], tMin, closest, &tmpHit)) {
            anything = true;
            closest = tmpHit.t;
            *outHit = tmpHit;
            *outID = i;
        }
    }
    for (unsigned int i = 0; i < triangles->used; i++) {
        if (triangle_hit(r, array_index(triangles, i), tMin, closest, &tmpHit)) {
            anything = true;
            closest = tmpHit.t;
            *outHit = tmpHit;
            *outID = 1;
        }
    }
    return anything;
}

static bool ray_scatter(Material* mat, Ray* ray, Hit* rec, vec3* attenuation, Ray* scattered, vec3* outLightE, int* inoutRayCount)
{
    *outLightE = vec3_uni(0.0f);
    if (mat->type == Lambert) {
        // random point inside unit sphere that is tangent to the hit point
        vec3 target = vec3_add(rec->pos, vec3_add(rec->normal, random_in_sphere()));
        *scattered = ray_new(rec->pos, vec3_normal(vec3_sub(target, rec->pos)));
        *attenuation = mat->albedo;
        
        // sample lights
        for (int i = 0; i < sphere_count; ++i) {
            Material* smat = &materials[i];
            if (smat->emissive.x <= 0.0 && smat->emissive.y <= 0.0 && smat->emissive.z <= 0.0) continue; // skip non-emissive
            if (mat == smat) continue; // skip self
            Sphere* s = &spheres[i];
            
            // create a random direction towards sphere
            // coord system for sampling: sw, su, sv
            vec3 sw = vec3_normal(vec3_sub(s->center, rec->pos));
            vec3 su = vec3_normal(vec3_cross(fabs(sw.x) > 0.01f ? vec3_new(0.0, 1.0, 0.0) : vec3_new(1.0, 0.0, 0.0), sw));
            vec3 sv = vec3_cross(sw, su);
            // sample sphere by solid angle
            float cosAMax = sqrtf(1.0f - s->radius * s->radius / vec3_sqmag(vec3_sub(rec->pos, s->center)));
            float eps1 = randf_norm(), eps2 = randf_norm();
            float cosA = 1.0f - eps1 + eps1 * cosAMax;
            float sinA = sqrtf(1.0f - cosA * cosA);
            float phi = 2 * M_PI * eps2;
            vec3 l = vec3_add(vec3_mult(su, cosf(phi) * sinA), vec3_add(vec3_mult(sv, sin(phi) * sinA), vec3_mult(sw, cosA)));
            l = vec3_normal(l);
            
            // shoot shadow ray
            Hit lightHit;
            int hitID;
            Ray r = {rec->pos, l};
            (*inoutRayCount)++;
            if (scene_hit(&r, kMinT, kMaxT, &lightHit, &hitID) && hitID == i) {
                float omega = 2.0 * M_PI * (1.0 - cosAMax);
                
                AssertUnit(ray->dir);
                vec3 nl = vec3_dot(rec->normal, ray->dir) < 0 ? rec->normal : vec3_neg(rec->normal);
                *outLightE = vec3_add(*outLightE, vec3_mult(vec3_prod(mat->albedo, smat->emissive), maxf(0.0f, vec3_dot(l, nl)) * omega / M_PI));
            }
        }
        return true;
    }
    else if (mat->type == Metal) {
        AssertUnit(ray->dir); AssertUnit(rec->normal);
        vec3 refl = vec3_reflect(ray->dir, rec->normal);
        // reflected ray, and random inside of sphere based on roughness
        *scattered = ray_new(rec->pos, vec3_normal(vec3_add(refl, vec3_mult(random_in_sphere(), mat->roughness))));
        *attenuation = mat->albedo;
        return vec3_dot(scattered->dir, rec->normal) > 0.0f;
    }
    else if (mat->type == Dielectric) {
        AssertUnit(ray->dir); 
        AssertUnit(rec->normal);
        vec3 outwardN;
        vec3 refl = vec3_reflect(ray->dir, rec->normal);
        float nint;
        *attenuation = vec3_uni(1.0f);
        vec3 refr;
        float reflProb;
        float cosine;
        if (vec3_dot(ray->dir, rec->normal) > 0.0f) {
            outwardN = vec3_neg(rec->normal);
            nint = mat->ri;
            cosine = mat->ri * vec3_dot(ray->dir, rec->normal);
        } else {
            outwardN = rec->normal;
            nint = 1.0f / mat->ri;
            cosine = -vec3_dot(ray->dir, rec->normal);
        }
        
        if (vec3_refract(ray->dir, outwardN, nint, &refr)) {
            reflProb = schlick(cosine, mat->ri);
        } else reflProb = 1.0f;
        
        if (randf_norm() < reflProb) *scattered = ray_new(rec->pos, vec3_normal(refl));
        else *scattered = ray_new(rec->pos, vec3_normal(refr));
    }
    else {
        *attenuation = vec3_new(1.0f, 0.0f, 1.0f);
        return false;
    }
    return true;
}

static vec3 ray_trace(Ray* ray, int depth, int* inoutRayCount)
{
    Hit rec;
    int id = 0;
    (*inoutRayCount)++;
    if (scene_hit(ray, kMinT, kMaxT, &rec, &id)) {
        Ray scattered;
        vec3 attenuation;
        vec3 lightE;
        Material* mat = &materials[id];
        if (depth < kMaxDepth && ray_scatter(mat, ray, &rec, &attenuation, &scattered, &lightE, inoutRayCount)) {
            return vec3_add(mat->emissive, vec3_add(lightE, vec3_prod(attenuation, ray_trace(&scattered, depth + 1, inoutRayCount))));
        } else return mat->emissive;
    } else {
        // Sky
        float t = 0.5f * (ray->dir.y + 1.0f);
        return vec3_mult(vec3_add(vec3_mult(vec3_new(0.7f, 0.7f, 1.0f), 1.0f - t), vec3_mult(vec3_new(0.5f, 0.7f, 1.0f), t)), 0.3f);
    }
}

typedef struct JobData {
    int frameCount;
    int screenWidth, screenHeight;
    float* backbuffer;
    Camera* cam;
    volatile int rayCount;
} JobData;

static JobData job;
static Camera cam;
static vec3 lookfrom = {0.0, 3.5, 6.0};
static vec3 lookat = {0.0, 0.0, 0.0};
static float distToFocus = 6.0f;
static float aperture = 0.1f;

#define CLMPF(x) ((x) * ((x) < 1.0) + (float)(x >= 1.0))

static void* frame_row_render(void* args)
{
    volatile static bool first = true;
    volatile static int frame = 1;
    bool check = first;
    first = false;

    uint32_t* uarg = args;
    uint32_t start = *(uarg++);
    uint32_t end = *uarg;

    static struct timespec time_start, time_end;
    if (check) clock_gettime(CLOCK_MONOTONIC, &time_start);

    float* backbuffer = job.backbuffer + start * job.screenWidth * 3;
    float invWidth = 1.0f / job.screenWidth;
    float invHeight = 1.0f / job.screenHeight;
    float lerpFac = (float)job.frameCount / (float)(job.frameCount + 1);
    lerpFac *= animate_smoothing;

    int rayCount = 0;
    for (uint32_t y = start; y < end; ++y) {
        for (int x = 0; x < job.screenWidth; ++x) {
            vec3 col = vec3_uni(0.0f);
            for (int s = 0; s < samples_per_pixel; s++) {
                float u = ((float)x + randf_norm()) * invWidth;
                float v = ((float)y + randf_norm()) * invHeight;
                Ray r = camera_ray(job.cam, u, v);
                col = vec3_add(col, ray_trace(&r, 0, &rayCount));
            }
            col = vec3_mult(col, 1.0f / (float)samples_per_pixel);
            col = vec3_new(sqrtf(col.x), sqrtf(col.y), sqrtf(col.z));
            
            vec3 prev = vec3_new(backbuffer[0], backbuffer[1], backbuffer[2]);
            col = vec3_add(vec3_mult(prev, lerpFac), vec3_mult(col, (1.0f - lerpFac)));
            backbuffer[0] = CLMPF(col.x);
            backbuffer[1] = CLMPF(col.y);
            backbuffer[2] = CLMPF(col.z);
            backbuffer += 3;
            if (check) {
                int samp = (y - start) * job.screenWidth + x + 1, off = (end - start) * job.screenWidth;
                printf("\rframe\t%d\t%.01f%%\t(%d\t/ %d)", frame, ((float)samp / (float)off) * 100.0f, samp, off);
            }
        }
    }
    if (check) {
        clock_gettime(CLOCK_MONOTONIC, &time_end);
        double time_elapsed = (time_end.tv_sec - time_start.tv_sec);
        printf("\t%fs\n", time_elapsed + (time_end.tv_nsec - time_start.tv_nsec) / 1000000000.0);
        frame++;
        first = true;
    }
    job.rayCount += rayCount;
    return NULL;
}

static void frame_render()
{   
    for (int i = 0; i < job.screenHeight; i++) {
        uint32_t start = i, end = i + 1;
        uint32_t args[2];
        args[0] = start;
        args[1] = end;
        frame_row_render(&args[0]);
    }
}

static void frame_render_threaded(int thread_count)
{
    uint32_t chunk = job.screenHeight / thread_count;
    uint32_t start = 0, end = chunk;

    pthread_t threads[thread_count - 1];
    uint32_t args[thread_count][2];
    for (int i = 0; i < thread_count - 1; i++) {
        args[i][0] = start;
        args[i][1] = end;
        pthread_create(&threads[i], NULL, *frame_row_render, &args[i][0]);
        start += chunk;
        end += chunk;
    }

    args[thread_count - 1][0] = start;
    args[thread_count - 1][1] = end;
    frame_row_render(&args[thread_count - 1][0]);

    for (int i = 0; i < thread_count - 1; i++) {
        pthread_join(threads[i], NULL);
    }
}

void scene_update(float time)
{
    cam.origin.z += time;
    cam.origin.x += time;
    cam.origin.y -= time;
}

int main(int argc, char** argv) 
{   
    int ray_count = 0, iters = 1, threads = 8;
    uint32_t width = 400, height = 400;
    char path[128], op[128];

    if (argc > 1) width = (uint32_t)atoi(argv[1]);
    if (argc > 2) height = (uint32_t)atoi(argv[2]);
    if (argc > 3) iters = atoi(argv[3]);
    if (argc > 4) samples_per_pixel = atoi(argv[4]);
    if (argc > 5) threads = atoi(argv[5]);

    if (!iters) {
        printf("frame count cannot be 0.\n");
        return EXIT_FAILURE;
    } else if (!threads) {
        printf("thread count cannot be 0.\n");
        return EXIT_FAILURE;
    } else if (!width || !height) {
        printf("width and height cannot be 0.\n");
        return EXIT_FAILURE;
    } else if (!samples_per_pixel) {
        printf("samples per pixel cannot be 0.\n");
        return EXIT_FAILURE;
    }

    bmp_t bmps[iters];

    //triangles = tracy_mesh_load("assets/suzanne.obj");
    triangles = array_new(1, sizeof(Triangle));
    Triangle tri = {{-1, 1.5, 2}, {1, 1.5, 2}, {0.5, 3, 1}, {0, 0, 0}};
    tri.n = triangle_norm(&tri);
    array_push(triangles, &tri);

    float* backbuffer = (float*)malloc(sizeof(float) * width * height * 3);
    cam = camera_new(lookfrom, lookat, vec3_new(0.0, 1.0, 0.0), 60, (float)width / (float)height, aperture, distToFocus);
    job.frameCount = iters;
    job.screenWidth = width;
    job.screenHeight = height;
    job.backbuffer = backbuffer;
    job.cam = &cam;
    job.rayCount = 0;

    printf("\ntracy is rendering the scene...\n");
    printf("threads:\t%d\nframes:\t\t%d\nwidth:\t\t%d\nheight:\t\t%d\nspp:\t\t%d\n", threads, iters, width, height, samples_per_pixel);
    printf("%s", string_separator);
    printf("frame:\tNº\t%%\t(px\t/ of\t)\ttime\n");
    printf("%s", string_separator);

    struct timespec time_start, time_end;
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    for (int i = 0; i < iters; i++) {
        int ray = 0;
        scene_update((float)i * 0.1);
        if (threads == 1) frame_render();
        else frame_render_threaded(threads);
        ray_count += ray;
        bmp_t bmp = bmp_new(width, height, 3);
        for (uint32_t j = 0; j < width * height * 3; j++) {
            bmp.pixels[j] = (uint8_t)(backbuffer[j] * 255.0f);
        }
        bmps[i] = bmp_flip_vertical(&bmp);
        bmp_free(&bmp);
    }
    clock_gettime(CLOCK_MONOTONIC, &time_end);
    double time_elapsed = (time_end.tv_sec - time_start.tv_sec);
    printf("%s", string_separator);
    printf("finished int %fs\n", time_elapsed + (time_end.tv_nsec - time_start.tv_nsec) / 1000000000.0);

    if (iters == 1) {
        strcpy(path, "out.png");
        bmp_write(path, &bmps[0]);
    } else {
        strcpy(path, "out.gif");
        gif_t* gif = bmp_to_gif(&bmps[0], iters);
        gif_file_write(path, gif);
    }
    free(backbuffer);

#ifdef __APPLE__
    strcpy(op, "open ");
#else 
    strcpy(op, "xdg-open ");
#endif
    strcat(op, path);
    system(op);
    return EXIT_SUCCESS;
}
