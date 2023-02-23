#include <tracy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static char* first_path = NULL;
static bool to_mp4 = false;
static int fps = 24;

static char* tstrdup(const char* str)
{
    const size_t size = strlen(str) + 1;
    char* ret = malloc(size);
    memcpy(ret, str, size);
    return ret;
}

static struct vector tracy_load_scenes(const struct vector* scene_files, const float aspect)
{
    struct vector scenes = vector_create(sizeof(Scene3D*));

    char** filenames = scene_files->data;
    const size_t count = scene_files->size;
    for (size_t i = 0; i < count; ++i) {
        Scene3D* scene = scene3D_load(filenames[i], aspect);
        if (scene) {
            vector_push(&scenes, &scene);
        }
    }

    return scenes;
}

static void tracy_to_mp4(const char* path)
{
    char command[BUFSIZ];
    sprintf(
        command,
        "pushd %s && ffmpeg -framerate 24 -pattern_type glob -i '*.png' -c:v libx264 -pix_fmt yuv420p %s.mp4 && popd", 
        path, 
        path
    );

    system(command);
}

static int tracy_render_scene(Render3D* restrict render, Scene3D* restrict scene, const char* restrict output_path)
{
    char image_name[BUFSIZ];
    char name[1024], fmt[8];
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
        
        bmp_t bmp = render3D_bmp(render, scene);
        bmp_write(output_path, &bmp);
        bmp_free(&bmp);

        if (!first_path) {
            first_path = tstrdup(output_path);
        }

        return EXIT_SUCCESS;
    }

    struct stat st;
    if (stat(name, &st) == -1) {
        mkdir(name, 0700);
    }

    for (uint32_t i = 0; i < frames; ++i) {
        sprintf(image_name, "%s/%s%.03u%s", name, name, i + 1, fmt);
        
        bmp_t bmp = render3D_bmp(render, scene);
        bmp_write(image_name, &bmp);
        bmp_free(&bmp);
        //scene3D_update(scene);

        ++render->timer;

        if (!first_path) {
            first_path = tstrdup(image_name);
        }
    }

    if (to_mp4) {
        tracy_to_mp4(name);
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
    struct vector scene_files = vector_create(sizeof(char*));
    Render3D render = render3D_new(400, 400, 4);
    char output_path[BUFSIZ] = "image.png";
    bool open = false;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-help")) {
            return tracy_help(0);
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
            else return tracy_error("Missing input for option -w. See -help for more information.\n");
        }
        else if (!strcmp(argv[i], "-h")) {
            if (++i < argc) {
                uint32_t h = (uint32_t)atoi(argv[i]);
                if (!h || h > 2160) {
                    return tracy_error("-h option cannot be smaller than 1 or larger than 2160.\n");
                } 
                else render.height = h;
            } 
            else return tracy_error("Missing input for option -h. See -help for more information.\n");
        }
        else if (!strcmp(argv[i], "-j")) {
            if (++i < argc) {
                uint32_t j = (uint32_t)atoi(argv[i]);
                if (!j || j > 128) {
                    return tracy_error("-j option cannot be smaller than 1 or larger than 128.\n");
                }
                else render.threads = j;
            }
            else return tracy_error("Missing input for option -j. See -help for more information.\n");
        }
        else if (!strcmp(argv[i], "-spp")) {
            if (++i < argc) {
                render.spp = (uint32_t)atoi(argv[i]);
                if (!render.spp) {
                    return tracy_error("-spp option cannot be smaller than 1.\n");
                }
            }
            else return tracy_error("Missing input for option -spp. See -help for more information.\n");
        }
        else if (!strcmp(argv[i], "-f")) {
            if (++i < argc) {
                render.frames = (uint32_t)atoi(argv[i]);
                if (!render.frames) {
                    return tracy_error("-f option cannot be smaller than 1.\n");
                }
            }
            else return tracy_error("Missing input for option -f. See -help for more information.\n");
        }
        else if (!strcmp(argv[i], "-o")) {
            if (++i < argc) {
                strcpy(output_path, argv[i]);
            }
            else return tracy_error("Missing input for option -o. See -help for more information.\n");
        }
        else if (!strcmp(argv[i], "-fps")) {
            if (++i < argc) {
                fps = atoi(argv[i]);
            }
            else return tracy_error("Missing input for option -fps. See -help for more information.\n");
        }
        else if (!strcmp(argv[i], "-open")) {
            open = true;
        }
        else if (!strcmp(argv[i], "-to-mp4")) {
            to_mp4 = true;
        }
        else vector_push(&scene_files, &argv[i]);
    }

    if (!scene_files.size) {
        tracy_error("Missing input scene file. See -help for more information.\n");
        return EXIT_FAILURE;
    }

    struct vector scenes = tracy_load_scenes(&scene_files, (float)render.width / (float)render.height);
    if (!scenes.size) {
        return tracy_error("No valid path to scene file was found.\n");
    }

    render3D_set(&render);
    tracy_log_render3D(&render);
#ifdef TRACY_PERF
    double time = time_clock();
#endif

    Scene3D** s = scenes.data;
    const size_t scene_count = scenes.size;
    for (size_t i = 0; i < scene_count; ++i) {
        if (tracy_render_scene(&render, s[i], output_path)) {
            break;
        }
    }
    
#ifdef TRACY_PERF
    tracy_log_time(time_clock() - time);
#endif

    for (size_t i = 0; i < scene_count; ++i) {
        scene3D_free(s[i]);
    }

    render3D_free(&render);
    vector_free(&scenes);
    vector_free(&scene_files);

    if (first_path) {
        if (open) {
            tracy_open_image(first_path);
        }
        free(first_path);
    }
    
    return EXIT_SUCCESS;
}
