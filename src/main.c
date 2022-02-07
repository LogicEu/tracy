#include <tracy.h>
#include <imgtool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* string_separator = "--------------------------------------------------------------------------------------------\n";

void scene_update(float time)
{
    lookfrom.x -= time;
    cam = camera_new(lookfrom, lookat, vec3_new(0.0, 1.0, 0.0), fov, (float)job.screenWidth / (float)job.screenHeight, aperture, distToFocus);
    Sphere* s = (Sphere*)spheres->data + 1;
    s->pos.y += time * 0.2;
}

int main(int argc, char** argv) 
{   
    int ray_count = 0, frames = 1, threads = 16;
    uint32_t width = 400, height = 400;
    char path[128], op[128];

    if (argc > 1) width = (uint32_t)atoi(argv[1]);
    if (argc > 2) height = (uint32_t)atoi(argv[2]);
    if (argc > 3) frames = atoi(argv[3]);
    if (argc > 4) samples_per_pixel = atoi(argv[4]);
    if (argc > 5) threads = atoi(argv[5]);

    if (!frames) {
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

    bmp_t bmps[frames];
    scene_init();

    float* backbuffer = (float*)malloc(sizeof(float) * width * height * 3);
    job.frameCount = frames;
    job.screenWidth = width;
    job.screenHeight = height;
    job.backbuffer = backbuffer;
    job.cam = &cam;
    job.rayCount = 0;

    printf("tracy is ready!\n");
    printf("threads:\t%d\nframes:\t\t%d\nwidth:\t\t%d\nheight:\t\t%d\nsamples:\t%d\n", threads, frames, width, height, samples_per_pixel);
    printf("rendering the scene...\n");
    printf("%s", string_separator);
    printf("frames\tNº\t%%\t(px\t/ of\t)\telapsed\t\testimate\tremaining\n");
    printf("%s", string_separator);

    double time = time_clock();
    for (int i = 0; i < frames; i++) {
        int ray = 0;
        scene_update((float)i * 0.02);
        frame_render(threads);
        ray_count += ray;
        bmp_t bmp = bmp_new(width, height, 3);
        for (uint32_t j = 0; j < width * height * 3; j++) {
            bmp.pixels[j] = (uint8_t)(backbuffer[j] * 255.0f);
        }
        bmps[i] = bmp_flip_vertical(&bmp);
        bmp_free(&bmp);
    }
    double time_elapsed = time_clock() - time;
    printf("%s", string_separator);
    int minutes = (int)(time_elapsed / 60.0);
    printf("total\t\t\t\t\t\t%.03fs\t%dm\t%f\n", time_elapsed, minutes, time_elapsed - 60.0 * minutes);
    printf("%s", string_separator);

    if (frames == 1) {
        strcpy(path, "out.png");
        bmp_write(path, &bmps[0]);
    } else {
        strcpy(path, "out.gif");
        gif_t* gif = bmp_to_gif(&bmps[0], frames);
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
