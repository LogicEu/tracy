#include <tracy.h>

Ray ray_new(vec3 orig, vec3 dir) 
{
    AssertUnit(dir);
    Ray r = {orig, dir};
    return r;
}

vec3 pointAt(Ray* r, float t) 
{ 
    return vec3_add(r->orig, vec3_mult(r->dir, t)); 
}