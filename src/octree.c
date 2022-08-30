#include <tracy.h>
#include <stdlib.h>

static void oct3D_insert(Oct3D* oct, const Tri3D* triangle);

static Oct3D* oct3D_children(const Box3D* box)
{
    static const vec3 offsets[8] = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},
        {1.0, 1.0, 0.0},
        {0.0, 1.0, 1.0},
        {1.0, 0.0, 1.0},
        {1.0, 1.0, 1.0}
    };

    Oct3D* children = malloc(sizeof(Oct3D) * 8);
    
    const Box3D half = {box->min, vec3_mult(_vec3_add(box->min, box->max), 0.5)};
    const vec3 d = _vec3_sub(half.max, half.min);
    
    for (int i = 0; i < 8; ++i) {
        children[i] = oct3D_create(box3D_move(half, _vec3_prod(d, offsets[i])));
    }
    
    return children;
}

static bool oct3D_children_insert(Oct3D* oct, const Tri3D* triangle)
{
    const Box3D b = box3D_from_triangle(triangle);
    size_t hitIndex = 0, i;
    for (i = 0; i < 8; ++i) {
        if (box3D_overlap(oct->children[i].box, b)) {
            if (hitIndex) {
                hitIndex = 0;
                break;
            }
            hitIndex = i + 1;
        }
    }

    if (hitIndex) {
        oct3D_insert(oct->children + hitIndex - 1, triangle);
    }

    return !!hitIndex;
}

static void oct3D_insert(Oct3D* oct, const Tri3D* triangle)
{
    if (oct->triangles.size < TRACY_OCTREE_LIMIT) {
        return array_push(&oct->triangles, triangle);
    }
    
    if (!oct->children) {
        oct->children = oct3D_children(&oct->box);
        const Tri3D* t = oct->triangles.data;
        for (size_t i = 0; i < oct->triangles.size; ++i, ++t) {
            if (oct3D_children_insert(oct, t)) {
                array_remove(&oct->triangles, i);
                --i, --t;
            }
        }
    }

    if (!oct3D_children_insert(oct, triangle)) {
        array_push(&oct->triangles, triangle);
    }
}

Oct3D oct3D_create(const Box3D box)
{
    Oct3D oct;
    oct.box = box;
    oct.children = NULL;
    oct.triangles = array_create(sizeof(Tri3D));
    return oct;
}

Oct3D oct3D_from_mesh(const Tri3D* triangles, const size_t count)
{
    Oct3D oct = oct3D_create(box3D_from_mesh((vec3*)triangles, count * 3));
    for (size_t i = 0; i < count; ++i) {
        oct3D_insert(&oct, triangles + i);
    }
    return oct;
}

bool oct3D_hit(const Oct3D* oct, const Ray3D* ray, Hit3D* hit, float closest)
{
    Hit3D tmpHit;
    bool anything = false;

    if (box3D_hit_fast(&oct->box, ray, &tmpHit.t) && tmpHit.t > TRACY_MIN_DIST && tmpHit.t < closest) {
        
        const Tri3D* t = oct->triangles.data;
        const size_t count = oct->triangles.size;

        for (size_t i = 0; i < count; i++) {
            if (tri3D_hit_fast(t++, ray, &tmpHit, closest)) {
                closest = tmpHit.t;
                *hit = tmpHit;
                anything = true;
            }
        }

        if (oct->children) {
            for (int i = 0; i < 8; ++i) {
                if (oct3D_hit(oct->children + i, ray, &tmpHit, closest)) {
                    closest = tmpHit.t;
                    *hit = tmpHit;
                    anything = true;
                }
            }
        }
        /*else if (oct->triangles.size && box3D_hit(&oct->box, ray, &tmpHit) && tmpHit.t > TRACY_MIN_DIST && tmpHit.t < closest) {
            closest = tmpHit.t;
            *hit = tmpHit;
            anything = true;
        }*/
    }

    return anything;
}

void oct3D_free(Oct3D* oct)
{
    if (oct->children) {
        for (int i = 0; i < 8; ++i) {
            oct3D_free(oct->children + i);
        }
        free(oct->children);
    }
    array_free(&oct->triangles);
}