#include <tracy.h>

void sphereUpdateDerivedData(Sphere* s) 
{ 
    s->invRadius = 1.0f / s->radius; 
}

bool HitSphere(Ray* r, Sphere* s, float tMin, float tMax, Hit* outHit)
{
    assert(s->invRadius == 1.0f / s->radius);
    AssertUnit(r->dir);
    vec3 oc = vec3_sub(r->orig, s->center);
    float b = vec3_dot(oc, r->dir);
    float c = vec3_dot(oc, oc) - s->radius*s->radius;
    float discr = b*b - c;
    if (discr > 0)
    {
        float discrSq = sqrtf(discr);
        
        float t = (-b - discrSq);
        if (t < tMax && t > tMin)
        {
            outHit->pos = pointAt(r, t);
            outHit->normal = vec3_mult(vec3_sub(outHit->pos, s->center), s->invRadius);
            outHit->t = t;
            return true;
        }
        t = (-b + discrSq);
        if (t < tMax && t > tMin)
        {
            outHit->pos = pointAt(r, t);
            outHit->normal = vec3_mult(vec3_sub(outHit->pos, s->center), s->invRadius);
            outHit->t = t;
            return true;
        }
    }
    return false;
}