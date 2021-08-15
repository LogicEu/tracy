#include <tracy.h>
#include <stdio.h>

vec3 vec3_reflect(vec3 v, vec3 n)
{
    return vec3_sub(v, vec3_mult(n, 2.0f * vec3_dot(v, n)));
}

bool vec3_refract(vec3 v, vec3 n, float nint, vec3* outRefracted)
{
    float dt = vec3_dot(v, n);
    float discr = 1.0f - nint * nint * (1.0f - dt * dt);
    if (discr > 0) {
        vec3 tmp = vec3_mult(vec3_sub(v, vec3_mult(n, dt)), nint);
        *outRefracted = vec3_sub(tmp, vec3_mult(n, sqrtf(discr)));
        return true;
    }
    return false;
}