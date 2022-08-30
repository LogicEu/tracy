#include <tracy.h>
#include <stdio.h>
#include <stdlib.h>

static const char* string_separator = "--------------------------------------------------------------------------------------------\n";

int tracy_error(const char* str)
{
    fprintf(stderr, "%s", str);
    return EXIT_FAILURE;
}

int tracy_version(void)
{
    fprintf(stdout, "tracy - the tiny path tracer - version 0.2.0\n");
    return EXIT_SUCCESS;
}

int tracy_help(const int runtime)
{
    fprintf(stdout, "tracy usage options:\n");
    fprintf(stdout, "<file_path>\t:Load scene file to render.\n");
    fprintf(stdout, "-o <file_path>\t:Set name of output file (*.png, *.jpg, *.ppm).\n");
    fprintf(stdout, "-w <number>\t:Set the width in pixels of output image.\n");
    fprintf(stdout, "-h <number>\t:Set the height in pixels of output image.\n");
    fprintf(stdout, "-j <number>\t:Set the number of threads to use.\n");
    fprintf(stdout, "-spp <number>\t:Set the number of samples per pixel to calculate.\n");
    if (!runtime) {
        fprintf(stdout, "-f <number>\t:Set the number of frames to output.\n");
        fprintf(stdout, "-open\t\t:Open first rendered image after done.\n");
        fprintf(stdout, "-to-mp4\t\t:Join multiple frames into a video.\n");
        fprintf(stdout, "-fps <number>\t:Set framerate of output video.\n");
    }
    fprintf(stdout, "-help\t\t:Print tracy's usage information.\n");
    fprintf(stdout, "-v, -version\t:Print tracy's version information.\n");
    return EXIT_SUCCESS;
}

int tracy_log_render3D(const Render3D* render)
{
    fprintf(stdout, "tracy's render information:\n");
    fprintf(stdout, "threads:\t%d\nframes:\t\t%d\nwidth:\t\t%d\nheight:\t\t%d\nsamples:\t%d\n", render->threads, render->frames, render->width, render->height, render->spp);
#ifdef TRACY_PERF
    fprintf(stdout, "rendering the scene...\n%s", string_separator);
    fprintf(stdout, "frames\tNº\t%%\t(px\t/ of\t)\telapsed\t\testimate\tremaining\n%s", string_separator);
#endif
    return EXIT_SUCCESS;
}

int tracy_log_time(const float time)
{
    const uint32_t minutes = (uint32_t)(time / 60.0);
    fprintf(stdout, "%s", string_separator);
    fprintf(stdout, "total\t\t\t\t\t\t%.03fs\t%um\t%f\n", time, minutes, time - 60.0 * minutes);
    fprintf(stdout, "%s", string_separator);
    return EXIT_SUCCESS;
}