#include <tracy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static const char* string_separator = "--------------------------------------------------------------------------------------------\n";
static char* first_path = NULL;

static int tracy_error(const char* restrict str)
{
    fprintf(stderr, "%s", str);
    return EXIT_FAILURE;
}

static int tracy_version()
{
    fprintf(stdout, "tracy - the tiny path tracer - version 0.1.0.\n");
    return EXIT_SUCCESS;
}

static int tracy_help()
{
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "<file_path>\t:Load scene file to render.\n");
    fprintf(stdout, "-o <file_path>\t:Set name of output file (*.png, *.jpg, *.ppm).\n");
    fprintf(stdout, "-w <number>\t:Set the width in pixels of output image.\n");
    fprintf(stdout, "-h <number>\t:Set the height in pixels of output image.\n");
    fprintf(stdout, "-j <number>\t:Set the number of threads to use.\n");
    fprintf(stdout, "-f <number>\t:Set the number of frames to output.\n");
    fprintf(stdout, "-spp <number>\t:Set the number of samples per pixel to calculate.\n");
    fprintf(stdout, "-h, -help\t:Print tracy's usage information.\n");
    fprintf(stdout, "-v, -version\t:Print tracy's version information.\n");
    return EXIT_SUCCESS;
}

static int tracy_log_render(const Render3D* render)
{
    fprintf(stdout, "tracy is ready!\n");
    fprintf(stdout, "threads:\t%d\nframes:\t\t%d\nwidth:\t\t%d\nheight:\t\t%d\nsamples:\t%d\n", render->threads, render->frames, render->width, render->height, render->spp);
    fprintf(stdout, "rendering the scene...\n%s", string_separator);
    fprintf(stdout, "frames\tNº\t%%\t(px\t/ of\t)\telapsed\t\testimate\tremaining\n%s", string_separator);
    return EXIT_SUCCESS;
}

static int tracy_log_time(const float time)
{
    const uint32_t minutes = (uint32_t)(time / 60.0);
    fprintf(stdout, "%s", string_separator);
    fprintf(stdout, "total\t\t\t\t\t\t%.03fs\t%um\t%f\n", time, minutes, time - 60.0 * minutes);
    fprintf(stdout, "%s", string_separator);
    return EXIT_SUCCESS;
}

static array_t tracy_load_scenes(const array_t* scene_files, const float aspect)
{
    array_t scenes = array_create(sizeof(Scene3D*));

    char** filenames = scene_files->data;
    const size_t count = scene_files->size;
    for (size_t i = 0; i < count; ++i) {
        Scene3D* scene = scene3D_load(filenames[i], aspect);
        if (scene) {
            array_push(&scenes, &scene);
        }
    }

    return scenes;
}

static int tracy_render_scene(const Render3D* restrict render, const Scene3D* restrict scene, const char* restrict output_path)
{
    char image_name[BUFSIZ];
    char name[BUFSIZ], fmt[8];
    strcpy(name, output_path);
    
    char* search = name;
    char* dot = NULL;
    while ((search = strchr(search, '.'))) {
        dot = search++;
    }

    if (!dot) {
        fprintf(stderr, "Output name must contain a file format like .png, .jpg or .ppm.\n");
        return EXIT_FAILURE;
    }

    strcpy(fmt, dot);
    *dot = '\0';

    const uint32_t frames = render->frames;
    if (frames == 1) {
        
        bmp_t bmp = render3D_render(render, scene);
        bmp_write(output_path, &bmp);
        bmp_free(&bmp);

        if (!first_path) {
            first_path = strdup(output_path);
        }

        return EXIT_SUCCESS;
    }

    struct stat st;
    if (stat(name, &st) == -1) {
        mkdir(name, 0700);
    }

    for (uint32_t i = 0; i < frames; i++) {
        sprintf(image_name, "%s/%s%.03u%s", name, name, i + 1, fmt);
        
        bmp_t bmp = render3D_render(render, scene);
        bmp_write(image_name, &bmp);
        bmp_free(&bmp);

        if (!first_path) {
            first_path = strdup(image_name);
        }
    }

    return EXIT_SUCCESS;
}

static void tracy_open_image(const char* path)
{
#ifdef __APPLE__
    static const char* open = "open ";
#else 
    static const char* open = "xdg-open ";
#endif

    char op[BUFSIZ];
    strcpy(op, open);
    strcat(op, path);
    system(op);
}

int main(int argc, char** argv) 
{   
    array_t scene_files = array_create(sizeof(char*));
    Render3D render = render3D_new(400, 400, 4);
    char output_path[BUFSIZ] = "image.png";
    bool open = false;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
            return tracy_help();
        }
        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "-version")) {
            return tracy_version();
        }
        else if (!strcmp(argv[i], "-w")) {
            if (++i < argc) {
                uint32_t w = (uint32_t)atoi(argv[i]);
                if (!w || w > 3840) {
                    return tracy_error("-w option cannot be smaller than 1 or larger than 3840.\n");
                } 
                else render.width = w;
            } 
            else return tracy_error("Missing input for option -w. See -h for more information.\n");
        }
        else if (!strcmp(argv[i], "-h")) {
            if (++i < argc) {
                uint32_t h = (uint32_t)atoi(argv[i]);
                if (!h || h > 2160) {
                    return tracy_error("-h option cannot be smaller than 1 or larger than 2160.\n");
                } 
                else render.height = h;
            } 
            else return tracy_error("Missing input for option -h. See -h for more information.\n");
        }
        else if (!strcmp(argv[i], "-j")) {
            if (++i < argc) {
                uint32_t j = (uint32_t)atoi(argv[i]);
                if (!j || j > 128) {
                    return tracy_error("-j option cannot be smaller than 1 or larger than 128.\n");
                }
                else render.threads = j;
            }
            else return tracy_error("Missing input for option -j. See -h for more information.\n");
        }
        else if (!strcmp(argv[i], "-spp")) {
            if (++i < argc) {
                render.spp = (uint32_t)atoi(argv[i]);
                if (!render.spp) {
                    return tracy_error("-spp option cannot be smaller than 1.\n");
                }
            }
            else return tracy_error("Missing input for option -spp. See -h for more information.\n");
        }
        else if (!strcmp(argv[i], "-f")) {
            if (++i < argc) {
                render.frames = (uint32_t)atoi(argv[i]);
                if (!render.frames) {
                    return tracy_error("-f option cannot be smaller than 1.\n");
                }
            }
            else return tracy_error("Missing input for option -f. See -h for more information.\n");
        }
        else if (!strcmp(argv[i], "-o")) {
            if (++i < argc) {
                strcpy(output_path, argv[i]);
            }
            else return tracy_error("Missing input for option -o. See -h for more information.\n");
        }
        else if (!strcmp(argv[i], "-open")) {
            open = true;
        }
        else array_push(&scene_files, &argv[i]);
    }

    if (!scene_files.size) {
        tracy_error("Missing input scene file. See -h for more information.\n");
        return EXIT_FAILURE;
    }

    array_t scenes = tracy_load_scenes(&scene_files, (float)render.width / (float)render.height);
    if (!scenes.size) {
        tracy_error("No valid path to scene file was found.\n");
        return EXIT_FAILURE;
    }

    render3D_set(&render);
    tracy_log_render(&render);
    double time = time_clock();

    Scene3D** s = scenes.data;
    const size_t scene_count = scenes.size;
    for (size_t i = 0; i < scene_count; ++i) {
        if (tracy_render_scene(&render, s[i], output_path)) {
            break;
        }
    }
    
    tracy_log_time(time_clock() - time);

    for (size_t i = 0; i < scene_count; ++i) {
        scene3D_free(s[i]);
    }

    render3D_free(&render);
    array_free(&scenes);
    array_free(&scene_files);

    if (first_path) {
        if (open) {
            tracy_open_image(first_path);
        }
        free(first_path);
    }
    
    return EXIT_SUCCESS;
}
