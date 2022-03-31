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

BoundingBox boundingBox;

array_t materials;

array_t spheres;
array_t sphmaterials;

array_t triangles;
array_t trimaterials;

vec3 skyColor = {1.0, 0.3, 1.0};
float skyMult = 0.2;

static void stars_init(int count)
{
    for (int i = 0; i < count; i++) {
        vec3 v = vec3_rand();
        v.y = absf(v.y);
        Sphere s = {vec3_mult(v, 100.0), 0.2};
        int mat = 2;
        array_push(&spheres, &s);
        array_push(&sphmaterials, &mat);
    }
    array_cut(&spheres);
    array_cut(&sphmaterials);
}

static void spheres_init()
{
    int mat = 0;
    Sphere s;
    spheres = array_create(sizeof(Sphere));
    sphmaterials = array_create(sizeof(int));

    s = sphere_new(vec3_new(0.0, -101.5, 0.0), 100.0);
    array_push(&spheres, &s);
    array_push(&sphmaterials, &mat);

    mat++;
    s = sphere_new(vec3_new(-8.0, 0.5, 0.0), 0.8);
    array_push(&spheres, &s);
    array_push(&sphmaterials, &mat);

    mat++;
    s = sphere_new(vec3_new(-2.0, 6.2, 0.0), 1.0);
    array_push(&spheres, &s);
    array_push(&sphmaterials, &mat);/*

    mat++;
    s = sphere_new(vec3_new(0.2, 1.0, 0.0), 1.0);
    array_push(&spheres, &s);
    array_push(&sphmaterials, &mat);

    s = sphere_new(vec3_new(0.0, 5.0, 10.0), 7.0);
    array_push(&spheres, &s);
    array_push(&sphmaterials, &mat);*/

    //stars_init(200);
}

static void materials_init()
{
    materials = array_create(sizeof(Material));
    Material m[] = {
        { Lambert, {0.8f, 0.8f, 0.8f}, {0, 0, 0}, 0.6, 0 },
        { Dielectric, {0.4f, 0.4f, 0.4f}, {0, 0, 0}, 0, 1.5f },
        { Lambert, {0.8f, 0.5f, 0.3f}, {25, 15, 5}, 0, 0 },
        { Metal, {1.0, 1.0, 1.0}, {0, 0, 0}, 0.01, 0 },
    };

    for (unsigned int i = 0; i < sizeof(m) / sizeof(m[0]); i++) {
        array_push(&materials, &m[i]);
    }
}

BoundingBox bounding_box_from_mesh(const vec3* restrict v, const size_t count)
{
    /*              min                max              */
    BoundingBox bb = {_vec3_uni(1000.0), _vec3_uni(-1000.0)};

    for (size_t i = 0; i < count; ++i, ++v) {
        
        if (v->x < bb.min.x) bb.min.x = v->x;
        if (v->y < bb.min.y) bb.min.y = v->y;
        if (v->z < bb.min.z) bb.min.z = v->z;

        if (v->x > bb.max.x) bb.max.x = v->x;
        if (v->y > bb.max.y) bb.max.y = v->y;
        if (v->z > bb.max.z) bb.max.z = v->z;

    }

    return bb;
}

static array_t tri3D_mesh_load(const char* path)
{
    mesh_t mesh = mesh_load(path);
    trimaterials = array_reserve(sizeof(int), mesh.vertices.size / 3);
    array_t arr = array_reserve(sizeof(Tri3D), mesh.vertices.size / 3);
    
    int mat = 0; 
    for (unsigned int i = 0; i < arr.capacity; i++) {
        array_push(&arr, array_index(&mesh.vertices, i * 3));
        array_push(&trimaterials, &mat);
    }
    
    mesh_free(&mesh);
    return arr;
}

static void tri3D_mesh_move(const array_t* restrict triangles, const vec3 trans)
{
    Tri3D* tri = triangles->data;
    for (Tri3D* end = tri + triangles->size; tri != end; tri++) {
        tri->a = vec3_add(tri->a, trans);
        tri->b = vec3_add(tri->b, trans);
        tri->c = vec3_add(tri->c, trans);
    }
}

static void triangles_init()
{
    triangles = tri3D_mesh_load("assets/suzanne.obj");
    tri3D_mesh_move(&triangles, vec3_new(0.0, 0.4, 0.0));
    boundingBox = bounding_box_from_mesh(triangles.data, triangles.size * 3);

    //int m = 0;
    //Tri3D tri;
    //triangles = array_create(sizeof(Tri3D));
    //trimaterials = array_create(sizeof(int));
    //array_push(&trimaterials, &m);

    //tri = tri3D_new(vec3_new(-6, -1.4, 4), vec3_new(-6, -1.4, -4), vec3_new(-3, 6, 0));
    //array_push(&triangles, &tri);
    //array_push(&trimaterials, &m);
}

void scene_init()
{
    spheres_init();
    materials_init();
    triangles_init();
}
