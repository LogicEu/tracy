#include <tracy.h>
#include <string.h>

int samples_per_pixel = 100;
float animate_smoothing = 0.01f;
bool light_sampling = true;

Cam3D cam;
vec3 lookfrom = {4.0, 1.0, 4.0};
vec3 lookat = {0.0, 0.0, 0.0};
float distToFocus = 8.0f;
float aperture = 0.1f;
float fov = 70.0;

array_t* materials;

array_t* spheres;
array_t* sphmaterials;

array_t* triangles;
array_t* trimaterials;

vec3 skyColor = {1.0, 0.3, 1.0};
float skyMult = 0.2;

static void stars_init(int count)
{
    for (int i = 0; i < count; i++) {
        vec3 v = vec3_rand();
        v.y = absf(v.y);
        Sphere s = {vec3_mult(v, 100.0), 0.2};
        int mat = 2;
        array_push(spheres, &s);
        array_push(sphmaterials, &mat);
    }
    array_cut(spheres);
    array_cut(sphmaterials);
}

static void spheres_init()
{
    int mat = 0;
    Sphere s;
    spheres = array_new(5, sizeof(Sphere));
    sphmaterials = array_new(5, sizeof(int));

    s = sphere_new(vec3_new(0.0, -101.5, 0.0), 100.0);
    array_push(spheres, &s);
    array_push(sphmaterials, &mat);

    mat++;
    s = sphere_new(vec3_new(-2.0, 0.5, 0.0), 0.8);
    array_push(spheres, &s);
    array_push(sphmaterials, &mat);

    mat++;
    s = sphere_new(vec3_new(-2.0, 4.2, 0.0), 1.0);
    array_push(spheres, &s);
    array_push(sphmaterials, &mat);

    mat++;
    s = sphere_new(vec3_new(0.2, 1.0, 0.0), 1.0);
    array_push(spheres, &s);
    array_push(sphmaterials, &mat);

    s = sphere_new(vec3_new(0.0, 5.0, 10.0), 7.0);
    array_push(spheres, &s);
    array_push(sphmaterials, &mat);

    //stars_init(200);
}

static void materials_init()
{
    materials = array_new(5, sizeof(Material));
    Material m[] = {
        { Lambert, {0.8f, 0.8f, 0.8f}, {0, 0, 0}, 0.6, 0 },
        { Dielectric, {0.4f, 0.4f, 0.4f}, {0, 0, 0}, 0, 1.5f },
        { Lambert, {0.8f, 0.5f, 0.3f}, {25, 15, 5}, 0, 0 },
        { Metal, {1.0, 1.0, 1.0}, {0, 0, 0}, 0, 0 },
    };

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
    
    Tri3D tri;
    triangles = array_new(2, sizeof(Tri3D));
    trimaterials = array_new(2, sizeof(int));

    int mat = 3;
    tri = tri3D_new(vec3_new(-6, -1.4, 4), vec3_new(-6, -1.4, -4), vec3_new(-3, 6, 0));
    array_push(triangles, &tri);
    array_push(trimaterials, &mat);
}

void scene_init()
{
    spheres_init();
    materials_init();
    triangles_init();
}
