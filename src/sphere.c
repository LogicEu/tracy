#include <tracy.h>

bool sphere_hit(Ray* ray, Sphere* sphere, float tMin, float tMax, Hit* outHit)
{
    AssertUnit(ray->dir);
    vec3 oc = vec3_sub(ray->orig, sphere->center);
    float b = vec3_dot(oc, ray->dir);
    float c = vec3_dot(oc, oc) - sphere->radius * sphere->radius;
    float discr = b * b - c;
    if (discr > 0.0f) {
        float discrSq = sqrtf(discr);
        
        float t = (-b - discrSq);
        if (t < tMax && t > tMin) {
            outHit->pos = ray_at(ray, t);
            outHit->normal = vec3_mult(vec3_sub(outHit->pos, sphere->center), 1.0f / sphere->radius);
            outHit->t = t;
            return true;
        }
        
        t = (-b + discrSq);
        if (t < tMax && t > tMin) {
            outHit->pos = ray_at(ray, t);
            outHit->normal = vec3_mult(vec3_sub(outHit->pos, sphere->center), 1.0f / sphere->radius);
            outHit->t = t;
            return true;
        }
    }
    return false;
}