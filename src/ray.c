#include <tracy.h>
#include <stdio.h>

Ray ray_new(vec3 orig, vec3 dir) 
{
    Ray ray = {orig, dir};
    return ray;
}

vec3 ray_at(Ray* restrict ray, float t) 
{ 
    return vec3_add(ray->orig, vec3_mult(ray->dir, t)); 
}