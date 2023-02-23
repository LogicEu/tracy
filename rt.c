#include <tracy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SPXE_APPLICATION
#include <spxe.h>

#define HALF_PI (M_PI * 0.5F)

static void cam3D_point(Cam3D* cam, vec3* dir, vec3* right, const int x, const int y)
{
    const float difx = -(float)x * 0.01;
    const float dify = (float)y * 0.01;

    dir->x = (sinf(difx) * cosf(dify));
    dir->y = sin(dify);
    dir->z = (cos(difx) * cosf(dify));

    *right = _vec3_new(sinf(difx - HALF_PI), 0.0f, cosf(difx - HALF_PI));
    cam->up = vec3_cross(*right, *dir);
    cam->lookAt = vec3_add(cam->lookFrom, *dir);
}

int main(const int argc, const char** argv) 
{   
    const char* scenePath = NULL;
    char outPath[BUFSIZ] = "image.png";
    Render3D render = render3D_new(200, 150, 1);
    
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-help")) {
            return tracy_help(1);
        }
        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "-version")) {
            return tracy_version();
        }
        else if (!strcmp(argv[i], "-w")) {
            if (++i < argc) {
                uint32_t w = (uint32_t)atoi(argv[i]);
                if (!w || w > 3840) {
                    return tracy_error("%s option cannot be smaller than 1 or larger than 3840.\n", argv[i]);
                } 
                else render.width = w;
            } 
            else return tracy_error("Missing input for option %s. See -help for more information.\n", argv[i]);
        }
        else if (!strcmp(argv[i], "-h")) {
            if (++i < argc) {
                uint32_t h = (uint32_t)atoi(argv[i]);
                if (!h || h > 2160) {
                    return tracy_error("%s option cannot be smaller than 1 or larger than 2160.\n", argv[i]);
                } 
                else render.height = h;
            } 
            else return tracy_error("Missing input for option %s. See -help for more information.\n", argv[i]);
        }
        else if (!strcmp(argv[i], "-j")) {
            if (++i < argc) {
                uint32_t j = (uint32_t)atoi(argv[i]);
                if (!j || j > 128) {
                    return tracy_error("%s option cannot be smaller than 1 or larger than 128.\n", argv[i]);
                }
                else render.threads = j;
            }
            else return tracy_error("Missing input for option %s. See -help for more information.\n", argv[i]);
        }
        else if (!strcmp(argv[i], "-spp")) {
            if (++i < argc) {
                render.spp = (uint32_t)atoi(argv[i]);
                if (!render.spp) {
                    return tracy_error("%s option cannot be smaller than 1.\n", argv[i]);
                }
            }
            else return tracy_error("Missing input for option %s. See -help for more information.\n", argv[i]);
        }
        else if (!strcmp(argv[i], "-o")) {
            if (++i < argc) {
                strcpy(outPath, argv[i]);
            }
            else return tracy_error("Missing input for option %s. See -help for more information.\n", argv[i]);
        }
        else scenePath = argv[i];
    }

    if (!scenePath) {
        tracy_error("Missing input scene file. See -help for more information.\n");
        return EXIT_FAILURE;
    }
    
    Scene3D* scene = scene3D_load(scenePath, (float)render.width / (float)render.height);
    if (!scene) {
        return EXIT_FAILURE;
    }

    tracy_log_render3D(&render);

    Px* pixbuf = spxeStart("tracy", 800, 600, render.width, render.height);
    render.buffer = (unsigned char*)pixbuf; 
    double T = spxeTime();

    vec3 dir, right;
    int mousex = 0, mousey = 0, x, y;
    spxeMousePos(&mousex, &mousey);
    cam3D_point(&scene->cam, &dir, &right, mousex, mousey);

    Material* const mat = scene->materials.data;

    while (spxeRun(pixbuf)) {
        double t = spxeTime();
        double dT = (t - T) * 2.0;
        T = t;

        if (spxeKeyPressed(ESCAPE) || spxeKeyPressed(Q)) {
            break;
        }
        if (spxeKeyDown(W)) {
            const vec3 d = vec3_mult(dir, dT);
            scene->cam.lookFrom = vec3_add(scene->cam.lookFrom, d);
            scene->cam.lookAt = vec3_add(scene->cam.lookAt, d);
            render.timer = 0;
        }
        if (spxeKeyDown(S)) {
            const vec3 d = vec3_mult(dir, dT);
            scene->cam.lookFrom = vec3_sub(scene->cam.lookFrom, d);
            scene->cam.lookAt = vec3_sub(scene->cam.lookAt, d);
            render.timer = 0;
        }
        if (spxeKeyDown(D)) {
            const vec3 d = vec3_mult(right, dT);
            scene->cam.lookFrom = vec3_add(scene->cam.lookFrom, d);
            scene->cam.lookAt = vec3_add(scene->cam.lookAt, d);
            render.timer = 0;
        }
        if (spxeKeyDown(A)) {
            const vec3 d = vec3_mult(right, dT);
            scene->cam.lookFrom = vec3_sub(scene->cam.lookFrom, d);
            scene->cam.lookAt = vec3_sub(scene->cam.lookAt, d);
            render.timer = 0;
        }
        if (spxeKeyDown(Z)) {
            const vec3 d = vec3_mult(scene->cam.up, dT);
            scene->cam.lookFrom = vec3_add(scene->cam.lookFrom, d);
            scene->cam.lookAt = vec3_add(scene->cam.lookAt, d);
            render.timer = 0;
        }
        if (spxeKeyDown(X)) {
            const vec3 d = vec3_mult(scene->cam.up, dT);
            scene->cam.lookFrom = vec3_sub(scene->cam.lookFrom, d);
            scene->cam.lookAt = vec3_sub(scene->cam.lookAt, d);
            render.timer = 0;
        }

        if (spxeKeyDown(M)) {
            mat->ri += 0.01;
            printf("Ri: %f\n", mat->ri);
            render.timer = 0;
        }
        if (spxeKeyDown(N)) {
            mat->ri -= 0.01;
            printf("Ri: %f\n", mat->ri);
            render.timer = 0;
        }
        if (spxeKeyDown(J)) {
            mat->roughness += 0.01;
            printf("Ro: %f\n", mat->roughness);
            render.timer = 0;
        }
        if (spxeKeyDown(K)) {
            mat->roughness -= 0.01;
            printf("Ro: %f\n", mat->roughness);
            render.timer = 0;
        }
        if (spxeKeyDown(LEFT_SHIFT)) {
            if (spxeKeyDown(R)) {
                if (spxeKeyDown(RIGHT) || spxeKeyDown(UP)) {
                    mat->albedo.x += 0.01;
                    render.timer = 0;
                }
                if (spxeKeyDown(LEFT) || spxeKeyDown(DOWN)) {
                    mat->albedo.x -= 0.01;
                    render.timer = 0;
                }
            }
            if (spxeKeyDown(G)) {
                if (spxeKeyDown(RIGHT) || spxeKeyDown(UP)) {
                    mat->albedo.y += 0.01;
                    render.timer = 0;
                }
                if (spxeKeyDown(LEFT) || spxeKeyDown(DOWN)) {
                    mat->albedo.y -= 0.01;
                    render.timer = 0;
                }
            }
            if (spxeKeyDown(B)) {
                if (spxeKeyDown(RIGHT) || spxeKeyDown(UP)) {
                    mat->albedo.z += 0.01;
                    render.timer = 0;
                }
                if (spxeKeyDown(LEFT) || spxeKeyDown(DOWN)) {
                    mat->albedo.z -= 0.01;
                    render.timer = 0;
                }
            }
        }
        if (spxeKeyDown(E)) {
            if (spxeKeyDown(R)) {
                if (spxeKeyDown(RIGHT) || spxeKeyDown(UP)) {
                    mat->emissive.x += 0.01;
                    render.timer = 0;
                }
                if (spxeKeyDown(LEFT) || spxeKeyDown(DOWN)) {
                    mat->emissive.x -= 0.01;
                    render.timer = 0;
                }
            }
            if (spxeKeyDown(G)) {
                if (spxeKeyDown(RIGHT) || spxeKeyDown(UP)) {
                    mat->emissive.y += 0.01;
                    render.timer = 0;
                }
                if (spxeKeyDown(LEFT) || spxeKeyDown(DOWN)) {
                    mat->emissive.y -= 0.01;
                    render.timer = 0;
                }
            }
            if (spxeKeyDown(B)) {
                if (spxeKeyDown(RIGHT) || spxeKeyDown(UP)) {
                    mat->emissive.z += 0.01;
                    render.timer = 0;
                }
                if (spxeKeyDown(LEFT) || spxeKeyDown(DOWN)) {
                    mat->emissive.z -= 0.01;
                    render.timer = 0;
                }
            }
        }
        if (spxeKeyPressed(U)) {
            ++mat->type;
            if (mat->type > Dielectric) {
                mat->type = Lambert;
            }
        }

        if (spxeKeyPressed(P)) {
            bmp_t bmp = {render.width, render.height, 4, render.buffer};
            bmp = bmp_flip_vertical(&bmp);
            bmp_write(outPath, &bmp);
            bmp_free(&bmp);
        }

        spxeMousePos(&x, &y);
        if (mousex != x || mousey != y) {
            cam3D_point(&scene->cam, &dir, &right, x, y);
            mousex = x;
            mousey = y;
            render.timer = 0;
        }
        
        if (!render.timer) {
            
            size_t id;
            Hit3D h;
            Ray3D r = cam3D_ray(&scene->cam, (float)render.width * 0.5 / (float)render.width, (float)render.height * 0.5 / (float)render.height);
            if (scene3D_hit(scene, &r, &h, &id)) {
                scene->cam.focusDist = h.t;
            }

            cam3D_update(&scene->cam);
        }

        render3D_render(&render, scene);
        ++render.timer;

        //printf("%lf\n", dT);
    }

    scene3D_free(scene);
    return spxeEnd(pixbuf);
}
