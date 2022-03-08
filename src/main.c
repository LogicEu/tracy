#include <tracy.h>
#include <imgtool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* string_separator = "--------------------------------------------------------------------------------------------\n";

int tracy_error(const char* restrict str)
{
    fprintf(stderr, "%s", str);
    return EXIT_FAILURE;
}

int tracy_help()
{
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "-w <number>\t:Set the width in pixels of output image.\n");
    fprintf(stdout, "-h <number>\t:Set the height in pixels of output image.\n");
    fprintf(stdout, "-j <number>\t:Set the number of threads to use.\n");
    fprintf(stdout, "-f <number>\t:Set the number of frames to output.\n");
    fprintf(stdout, "-spp <number>\t:Set the number of samples per pixel to calculate.\n");
    return EXIT_SUCCESS;
}

void tracy_scene_update(float time)
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

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-help")) {
            return tracy_help();
        }
        else if (!strcmp(argv[i], "-w")) {
            if (++i < argc) {
                uint32_t w = (uint32_t)atoi(argv[i]);
                if (w < 1 || w > 3840) {
                    return tracy_error("-w option cannot be smaller than 1 or larger than 3840\n");
                } 
                else width = w;
            } 
            else return tracy_error("missing input for option -w\n");
        }
        else if (!strcmp(argv[i], "-h")) {
            if (++i < argc) {
                uint32_t h = (uint32_t)atoi(argv[i]);
                if (h < 1 || h > 2160) {
                    return tracy_error("-h option cannot be smaller than 1 or larger than 2160\n");
                } 
                else height = h;
            } 
            else return tracy_error("missing input for option -h\n");
        }
        else if (!strcmp(argv[i], "-j")) {
            if (++i < argc) {
                uint32_t j = (uint32_t)atoi(argv[i]);
                if (j < 1 || j > 128) {
                    return tracy_error("-j option cannot be smaller than 1 or larger than 128\n");
                }
                else threads = j;
            }
            else return tracy_error("missing input for option -j\n");
        }
        else if (!strcmp(argv[i], "-spp")) {
            if (++i < argc) {
                int spp = atoi(argv[i]);
                if (spp < 1) {
                    return tracy_error("-spp option cannot be smaller than 1\n");
                }
                else samples_per_pixel = spp;
            }
            else return tracy_error("missing input for option -spp\n");
        }
        else if (!strcmp(argv[i], "-f")) {
            if (++i < argc) {
                int f = atoi(argv[i]);
                if (f < 1) {
                    return tracy_error("-f option cannot be smaller than 1\n");
                }
                else frames = f;
            }
            else return tracy_error("missing input for option -f\n");
        }
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
        tracy_scene_update((float)i * 0.02);
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
