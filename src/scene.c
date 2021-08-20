#include <tracy.h>
#include <string.h>

int samples_per_pixel = 100;
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
        {{0.0, -101.5, 0.0}, 100}, //floor
        {{-1.6, 0.5, 0.0}, 0.8f}, // glass
        {{-2.0, 4.2, 0.0}, 1.0f}, //light 
        {{0.2, 1.0, 0.0}, 1.0},
        {{0.0, 5.0, 10.0}, 7.0},
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
        tri.n = tri3D_norm(&tri);
        array_push(arr, &tri);
    }
    array_cut(arr);
    mesh_free(mesh);
    return arr;
}

static void tri3D_mesh_move(const array_t* restrict triangles, const vec3 trans)
{
    Tri3D* tri = triangles->data;
    for (Tri3D* end = tri + triangles->used; tri != end; tri++) {
        tri->a = vec3_add(tri->a, trans);
        tri->b = vec3_add(tri->b, trans);
        tri->c = vec3_add(tri->c, trans);
    }
}

static void triangles_init()
{
    //triangles = tri3D_mesh_load("assets/suzanne.obj");
    //tri3D_mesh_move(triangles, vec3_new(0.0, 0.8, 0.0));
    
    triangles = array_new(1, sizeof(Tri3D));
    {
        Tri3D tri = {{-0.8, -1.4, 1.3}, {-0.8, -1.4, -1.3}, {-3, 0.4, 0}, {0, 0, 0}};
        tri.n = tri3D_norm(&tri);
        array_push(triangles, &tri);
    }
    {
        Tri3D tri = {{-6, -1.4, 4}, {-6, -1.4, -4}, {-3, 6, 0}, {0, 0, 0}};
        tri.n = tri3D_norm(&tri);
        array_push(triangles, &tri);
    }
}

void scene_init()
{
    spheres_init();
    materials_init();
    triangles_init();
}