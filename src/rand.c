#include <tracy.h>

vec3 RandomInUnitDisk()
{
    vec3 p;
    do {
        p = vec3_sub(vec3_mult(vec3_new(randf_norm(), randf_norm(), 0.0), 2.0f), vec3_new(1.0, 1.0, 0.0));
    } while (vec3_dot(p, p) >= 1.0);
    return p;
}

vec3 RandomInUnitSphere()
{
    vec3 p;
    do {
        p = vec3_sub(vec3_mult(vec3_new(randf_norm(), randf_norm(), randf_norm()), 2.0f), vec3_new(1.0, 1.0, 1.0));
    } while (vec3_sqmag(p) >= 1.0);
    return p;
}