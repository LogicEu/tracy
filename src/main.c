#include <tracy.h>
#include <imgtool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char* string_separator = "--------------------------------------------------------------------------------------------\n";

int samples_per_pixel = 400;
float animate_smoothing = 0.01f;
bool light_sampling = true;

Cam3D cam;
vec3 lookfrom = {4.0, 1.0, 4.0};
vec3 lookat = {-1.8, 0.5, 0.0};
float distToFocus = 8.0f;
float aperture = 0.1f;
float fov = 40.0;

array_t* triangles;
array_t* spheres;
array_t* materials;

static void spheres_init()
{
    Sphere s[] = {
        {{0.0, -100.5, 0.0}, 100}, //floor
        {{-1.6, 0.5, 0.0}, 0.8f}, // glass
        {{-2.0, 4.2, 0.0}, 1.0f}, //light 
        {{0.2, 1.0, 0.0}, 1.0},
        {{0.0, 5.0, 9.0}, 7.0},
    };
    
    spheres = array_new(sizeof(s) / sizeof(s[0]), sizeof(Sphere));
    for (unsigned int i = 0; i < spheres->size; i++) {
        array_push(spheres, &s[i]);
    }
}

static void materials_init()
{
    Material m[] = {
        { Lambert, {0.8f, 0.8f, 0.8f}, {0, 0, 0}, 0.6, 0 },
        { Dielectric, {0.4f, 0.4f, 0.4f}, {0, 0, 0}, 0, 1.5f },
        { Lambert, {0.8f, 0.5f, 0.3f}, {25, 15, 5}, 0, 0 },
        { Metal, {1.0, 1.0, 1.0}, {0, 0, 0}, 0, 0 },
        { Metal, {1.0, 1.0, 1.0}, {0, 0, 0}, 0, 0 }
    };
    
    materials = array_new(sizeof(m) / sizeof(m[0]), sizeof(Material));
    for (unsigned int i = 0; i < materials->size; i++) {
        array_push(materials, &m[i]);
    }
}

static array_t* tri3D_mesh_load(const char* path)
{
    mesh_t* mesh = mesh_load(path);
    array_t* arr = array_new(mesh->vertices->used / 3, sizeof(Tri3D));
    for (unsigned int i = 0; i < arr->size; i++) {
        Tri3D tri;
        memcpy(&tri, array_index(mesh->vertices, i * 3), sizeof(vec3) * 3);
        tri.a.y += 0.5; tri.b.y += 0.5; tri.c.y += 0.5;
        tri.n = tri3D_norm(&tri);
        array_push(arr, &tri);
    }
    array_cut(arr);
    mesh_free(mesh);
    return arr;
}

static void triangles_init()
{
    //triangles = tri3D_mesh_load("assets/suzanne.obj");
    triangles = array_new(1, sizeof(Tri3D));
    {
        Tri3D tri = {{-3, -0.4, 1.3}, {-3, -0.4, -1.3}, {-3, 2, 0}, {0, 0, 0}};
        tri.n = tri3D_norm(&tri);
        array_push(triangles, &tri);
    }
    {
        Tri3D tri = {{-6, -0.4, 4}, {-6, -0.4, -4}, {-3, 6, 0}, {0, 0, 0}};
        tri.n = tri3D_norm(&tri);
        array_push(triangles, &tri);
    }
}

void scene_update(float time)
{
    cam = camera_new(lookfrom, lookat, vec3_new(0.0, 1.0, 0.0), fov, (float)job.screenWidth / (float)job.screenHeight, aperture, distToFocus);
    lookfrom.z += time;
    distToFocus += time;
}

int main(int argc, char** argv) 
{   
    int ray_count = 0, iters = 1, threads = 16;
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
    spheres_init();
    materials_init();
    triangles_init();

    printf("tracy is ready!\n");

    float* backbuffer = (float*)malloc(sizeof(float) * width * height * 3);
    job.frameCount = iters;
    job.screenWidth = width;
    job.screenHeight = height;
    job.backbuffer = backbuffer;
    job.cam = &cam;
    job.rayCount = 0;

    printf("threads:\t%d\nframes:\t\t%d\nwidth:\t\t%d\nheight:\t\t%d\nsamples:\t%d\n", threads, iters, width, height, samples_per_pixel);
    printf("rendering the scene...\n");
    printf("%s", string_separator);
    printf("frames\tNº\t%%\t(px\t/ of\t)\telapsed\t\testimate\tremaining\n");
    printf("%s", string_separator);

    struct timespec time_start, time_end;
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    for (int i = 0; i < iters; i++) {
        int ray = 0;
        scene_update((float)i * 0.02);
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
    double time_elapsed = (time_end.tv_sec - time_start.tv_sec) + (time_end.tv_nsec - time_start.tv_nsec) / 1000000000.0;
    printf("%s", string_separator);
    int minutes = (int)(time_elapsed / 60.0);
    printf("total\t\t\t\t\t\t%.03fs\t%dm\t%f\n", time_elapsed, minutes, time_elapsed - 60.0 * minutes);
    printf("%s", string_separator);

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
