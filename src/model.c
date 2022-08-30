#include <tracy.h>
#include <stdlib.h>

static array_t tri3D_mesh_load(const char* path)
{
    Mesh3D mesh = mesh3D_load(path);
    array_t arr = array_move(&mesh.vertices);
    mesh3D_free(&mesh);
    array_restructure(&arr, sizeof(Tri3D));
    return arr;
}

Model3D* model3D_load(const char* filename)
{
    array_t mesh = tri3D_mesh_load(filename);
    if (!mesh.size) {
        return NULL;
    }

    Model3D* model = malloc(sizeof(Model3D));
    model->triangles = mesh;
    model->octree = oct3D_from_mesh(mesh.data, mesh.size);

    return model;
}

void model3D_move(const Model3D* model, const vec3 trans)
{
    vec3* v = model->triangles.data;
    for (const vec3* end = v + model->triangles.size * 3; v != end; ++v) {
        *v = vec3_add(*v, trans);
    }
}

void model3D_scale(const Model3D* model, const float scale)
{
    vec3* v = model->triangles.data;
    for (const vec3* end = v + model->triangles.size * 3; v != end; ++v) {
        *v = vec3_mult(*v, scale);
    }
}

void model3D_scale3D(const Model3D* model, const vec3 scale)
{
    vec3* v = model->triangles.data;
    for (const vec3* end = v + model->triangles.size * 3; v != end; ++v) {
        v->x *= scale.x;
        v->y *= scale.y;
        v->z *= scale.z;
    }
}

void model3D_free(Model3D* model)
{
    if (!model) return;
    array_free(&model->triangles);
    oct3D_free(&model->octree);
    free(model);
}
