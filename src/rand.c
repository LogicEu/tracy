#include <tracy.h>

vec3 random_in_disk()
{
    vec3 p = vec3_sub(vec3_mult(vec3_new(randf_norm(), randf_norm(), 0.0), 2.0f), vec3_new(1.0, 1.0, 0.0));
    return p;
}

vec3 random_in_sphere()
{
    vec3 p = vec3_sub(vec3_mult(vec3_new(randf_norm(), randf_norm(), randf_norm()), 2.0f), vec3_new(1.0, 1.0, 1.0));
    return p;
}