#include <tracy.h>
#include <string.h>

array_t* tracy_mesh_load(const char* path)
{
    mesh_t* mesh = mesh_load(path);
    array_t* arr = array_new(mesh->vertices->used / 3, sizeof(Triangle));
    for (int i = 0; i < arr->size; i++) {
        Triangle tri;
        memcpy(&tri, array_index(mesh->vertices, i * 3), sizeof(vec3) * 3);
        tri.n = *(vec3*)array_index(mesh->normals, i * 3);
        array_push(arr, &tri);
    }
    array_cut(arr);
    mesh_free(mesh);
    return arr;
}