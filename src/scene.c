#include <tracy.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static enum MatType material_type_parse(const char* token)
{
    if (!strcmp(token, "l") || !strcmp(token, "L") || !strcmp(token, "lambert") || !strcmp(token, "Lambert")) {
        return Lambert;
    }
    else if (!strcmp(token, "m") || !strcmp(token, "M") || !strcmp(token, "metal") || !strcmp(token, "Metal")) {
        return Metal;
    }
    else if (!strcmp(token, "d") || !strcmp(token, "D") || !strcmp(token, "dielectric") || !strcmp(token, "Dielectric")) {
        return Dielectric;
    }
    return Invisible;
}

static Scene3D* scene3D_new(void)
{
    Scene3D* scene = malloc(sizeof(Scene3D));

    scene->materials = array_create(sizeof(Material));
    scene->spheres = array_create(sizeof(Sphere));
    scene->sphere_materials = array_create(sizeof(size_t));
    scene->triangles = array_create(sizeof(Tri3D));
    scene->triangle_materials = array_create(sizeof(size_t)); 
    scene->models = array_create(sizeof(Model3D*));
    scene->background_color = vec3_new(0.2, 0.2, 1.0);
    
    return scene;
}

Scene3D* scene3D_load(const char* filename, const float aspect)
{
    static const char* symbols = "\n ,:;[]{}()<>";

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Could not open file '%s'.\n", filename);
        return NULL;
    }

    Scene3D* scene = scene3D_new();
    
    vec3 lookfrom = vec3_new(0.0, 0.0, -2.0);
    vec3 lookat = vec3_uni(0.0);
    vec3 up = vec3_new(0.0, 1.0, 0.0);
    
    float fov = 45.0;
    float aperture = 0.1;
    float focus = 2.0;

    char line[BUFSIZ];
    while ((fgets(line, BUFSIZ, file))) {

        char* token = strtok(line, symbols);
        if (!token || *token == '#') {
            continue;
        }
        else if (!strcmp(token, "model") || !strcmp(token, "load")) {
            
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }
            
            Model3D* model = model3D_load(token);
            if (!model) {
                continue;
            }

            while ((token = strtok(NULL, symbols))) {
                if (!strcmp(token, "#")) {
                    break;
                }
                else if (!strcmp(token, "scale")) {
                    vec3 scale = vec3_uni(1.0);
                    const size_t size = sizeof(vec3) / sizeof(float);
                    float *f = (float*)&scale;
                    for (size_t i = 0; i < size; ++i) {
                        if (!(token = strtok(NULL, symbols))) {
                            break;
                        }
                        sscanf(token, "%f", f++);
                    }
                    model3D_scale3D(model, scale);
                } 
                else if (!strcmp(token, "move")) {
                    vec3 move = vec3_uni(0.0);
                    const size_t size = sizeof(vec3) / sizeof(float);
                    float *f = (float*)&move;
                    for (size_t i = 0; i < size; ++i) {
                        if (!(token = strtok(NULL, symbols))) {
                            break;
                        }
                        sscanf(token, "%f", f++);
                    }
                    model3D_move(model, move);
                }

                if (!token) {
                    break;
                }
            }
            
            model->bounds = box3D_from_mesh(model->triangles.data, model->triangles.size * 3);
            array_push(&scene->models, &model);

        }
        else if (!strcmp(token, "sky") || !strcmp(token, "background")) {
            
            float *f = (float*)&scene->background_color;
            const size_t size = sizeof(vec3) / sizeof(float);
            
            for (size_t i = 0; i < size; ++i) {
                if (!(token = strtok(NULL, symbols))) {
                    break;
                }
                sscanf(token, "%f", f++);
            }
        }
        else if (!strcmp(token, "m") || !strcmp(token, "mat") || !strcmp(token, "material")) {
            
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }

            const enum MatType type = material_type_parse(token);
            Material m = {type, {0.5, 0.5, 0.5}, {0.0, 0.0, 0.0}, 0.0, 0.0};

            float *f = (float*)&m.albedo;
            const size_t size = (sizeof(Material) - sizeof(enum MatType)) / sizeof(float);

            for (size_t i = 0; i < size; ++i) {
                if (!(token = strtok(NULL, symbols))) {
                    break;
                }
                sscanf(token, "%f", f++);
            }
            array_push(&scene->materials, &m);
        }
        else if (!strcmp(token, "s") || !strcmp(token, "sphere")) {
            
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }

            size_t material_index;
            sscanf(token, "%zu", &material_index);
            array_push(&scene->sphere_materials, &material_index);
            
            Sphere s = {{0.0, 0.0, 0.0}, 1.0};
            float *f = (float*)&s;  
            const size_t size = sizeof(Sphere) / sizeof(float);

            for (size_t i = 0; i < size; ++i) {
                if (!(token = strtok(NULL, symbols))) {
                    break;
                }
                sscanf(token, "%f", f++);
            }
            array_push(&scene->spheres, &s);
        }
        else if (!strcmp(token, "t") || !strcmp(token, "triangle")) {
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }

            size_t material_index;
            sscanf(token, "%zu", &material_index);
            array_push(&scene->triangle_materials, &material_index);
            
            Tri3D tri = {_vec3_uni(0.0), _vec3_uni(0.0), _vec3_uni(0.0)};
            float *f = (float*)&tri;
            const size_t size = sizeof(Tri3D) / sizeof(float);

            for (size_t i = 0; i < size; ++i) {
                if (!(token = strtok(NULL, symbols))) {
                    break;
                }
                sscanf(token, "%f", f++);
            }
            array_push(&scene->triangles, &tri);
        }
        else if (!strcmp(token, "lookfrom") || !strcmp(token, "origin")) {
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }

            float *f = (float*)&lookfrom;
            const size_t size = sizeof(vec3) / sizeof(float);
            for (size_t i = 0; i < size; ++i) {
                sscanf(token, "%f", f++);
                if (!(token = strtok(NULL, symbols))) {
                    break;
                }
            }         
        }
        else if (!strcmp(token, "lookat") || !strcmp(token, "direction")) {
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }

            float *f = (float*)&lookat;
            const size_t size = sizeof(vec3) / sizeof(float);
            for (size_t i = 0; i < size; ++i) {
                sscanf(token, "%f", f++);
                if (!(token = strtok(NULL, symbols))) {
                    break;
                }
            }         
        }
        else if (!strcmp(token, "up")) {
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }

            float *f = (float*)&up;
            const size_t size = sizeof(vec3) / sizeof(float);
            for (size_t i = 0; i < size; ++i) {
                sscanf(token, "%f", f++);
                if (!(token = strtok(NULL, symbols))) {
                    break;
                }
            }         
        }
        else if (!strcmp(token, "fov")) {
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }

            sscanf(token, "%f", &fov);            
        }
        else if (!strcmp(token, "aperture")) {
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }

            sscanf(token, "%f", &aperture);            
        }
        else if (!strcmp(token, "focus")) {
            token = strtok(NULL, symbols);
            if (!token) {
                fprintf(stderr, "tracy error: No argument for '%s' command.\n", token);
                fclose(file);
                return NULL;
            }

            sscanf(token, "%f", &focus);            
        }
    }
    fclose(file);

    scene->cam = cam3D_new(lookfrom, lookat, up, fov, aspect, aperture, focus);
    return scene;
}

void scene3D_write(const char* filename, const Scene3D* scene)
{
    static const char* matname[4] = {
        "undefined",
        "lambert",
        "metal",
        "dielectric"
    };

    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "tracy error: Could not write file '%s'.\n", filename);
        return;
    }

    fprintf(file, "# tracy 3D scene\n\n# camera:\n");
    fprintf(file, "fov %f\n", scene->cam.params.fov);
    fprintf(file, "focus %f\n", scene->cam.params.focusDist);
    fprintf(file, "aperture %f\n\n", scene->cam.params.aperture);
    fprintf(file, "up {%f, %f, %f}\n", scene->cam.params.up.x, scene->cam.params.up.y, scene->cam.params.up.z);
    fprintf(file, "lookfrom {%f, %f, %f}\n", scene->cam.params.lookFrom.x, scene->cam.params.lookFrom.y, scene->cam.params.lookFrom.z);
    fprintf(file, "lookat {%f, %f, %f}\n\n", scene->cam.params.lookAt.x, scene->cam.params.lookAt.y, scene->cam.params.lookAt.z);

    fprintf(file, "# sky color\nsky {%f, %f, %f}\n", scene->background_color.x, scene->background_color.y, scene->background_color.z);

    const size_t matsize = scene->materials.size;
    if (matsize) {
        fprintf(file, "# materials\n");
        Material* mat = scene->materials.data;
        for (size_t i = 0; i < matsize; ++i, ++mat) {
            fprintf(
                file, 
                "material %s {{%f, %f, %f}, {%f, %f, %f}, %f, %f}\n",
                matname[mat->type],
                mat->albedo.x,
                mat->albedo.y,
                mat->albedo.z,
                mat->emissive.x,
                mat->emissive.y,
                mat->emissive.z,
                mat->roughness,
                mat->ri
            );
        }
    }

    const size_t sphsize = scene->spheres.size;
    if (sphsize) {
        fprintf(file, "# spheres\n");
        size_t* mat = scene->sphere_materials.data;
        Sphere* s = scene->spheres.data;
        for (size_t i = 0; i < sphsize; ++i, ++s, ++mat) {
            fprintf(
                file, 
                "sphere %zu {%f, %f, %f, %f}\n",
                *mat,
                s->pos.x,
                s->pos.y,
                s->pos.z,
                s->radius
            );
        }
    }

    const size_t trisize = scene->triangles.size;
    if (trisize) {
        fprintf(file, "# triangles\n");
        size_t* mat = scene->triangle_materials.data;
        Tri3D* t = scene->triangles.data;
        for (size_t i = 0; i < trisize; ++i, ++t, ++mat) {
            fprintf(
                file, 
                "triangle %zu {{%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}}\n",
                *mat,
                t->a.x,
                t->a.y,
                t->a.z,
                t->b.x,
                t->b.y,
                t->b.z,
                t->c.x,
                t->c.y,
                t->c.z
            );
        }
    }

    fclose(file);
}

bool scene3D_hit(const Scene3D* restrict scene, const Ray3D* restrict ray, Hit3D* restrict outHit, size_t* restrict outID)
{
    Hit3D tmpHit;
    float closest = TRACY_MAX_DIST;
    bool anything = false;

    const size_t model_count = scene->models.size;
    const Model3D** models = scene->models.data;
    for (size_t i = 0; i < model_count; ++i) {
        if (box3D_hit(&models[i]->bounds, ray, &tmpHit) && tmpHit.t > TRACY_MIN_DIST && tmpHit.t < closest) {
            const Tri3D* tri = models[i]->triangles.data;
            const size_t triangle_count = models[i]->triangles.size;
            for (size_t j = 0; j < triangle_count; j++) {
                if (tri3D_hit(tri++, ray, &tmpHit) && tmpHit.t > TRACY_MIN_DIST && tmpHit.t < closest) {
                    closest = tmpHit.t;
                    *outHit = tmpHit;
                    *outID = 0;
                    anything = true;
                }
            }
        }
    }

    size_t* indices = scene->triangle_materials.data;
    const size_t triangle_count = scene->triangles.size;
    const Tri3D* tri = scene->triangles.data;
    for (size_t i = 0; i < triangle_count; i++) {
        if (tri3D_hit(tri++, ray, &tmpHit) && tmpHit.t > TRACY_MAX_DEPTH && tmpHit.t < closest) {
            closest = tmpHit.t;
            *outHit = tmpHit;
            *outID =  indices[i];
            anything = true;
        }
    }

    indices = scene->sphere_materials.data;
    const size_t sphere_count = scene->spheres.size;
    const Sphere* spheres = scene->spheres.data;
    for (size_t i = 0; i < sphere_count; ++i) {
        if (sphere_hit(spheres[i], ray, &tmpHit) && tmpHit.t > TRACY_MIN_DIST && tmpHit.t < closest) {
            closest = tmpHit.t;
            *outHit = tmpHit;
            *outID = indices[i];
            anything = true;
        }
    }

    return anything;
}

void scene3D_free(Scene3D* scene)
{
    if (!scene) return;

    Model3D** models = scene->models.data;
    const size_t model_count = scene->models.size;
    for (size_t i = 0; i < model_count; ++i) {
        model3D_free(models[i]);
    }

    array_free(&scene->models);
    array_free(&scene->triangles);
    array_free(&scene->triangle_materials);
    array_free(&scene->spheres);
    array_free(&scene->sphere_materials);
    array_free(&scene->materials);

    free(scene);
}