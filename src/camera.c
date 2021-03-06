#include <tracy.h>

Cam3D cam3D_new(const vec3 lookFrom, const vec3 lookAt, const vec3 up, const float fov, const float aspect, const float aperture, const float focusDist)
{
    Cam3D cam;
    cam.lensRadius = aperture * 0.5;
    float theta = fov * M_PI / 180.0;
    float halfHeight = tanf(theta * 0.5);
    float halfWidth = aspect * halfHeight;
    cam.origin = lookFrom;
    cam.w = vec3_normal(vec3_sub(lookFrom, lookAt));
    cam.u = vec3_normal(vec3_cross(up, cam.w));
    cam.v = vec3_cross(cam.w, cam.u);
    cam.lowerLeftCorner = vec3_sub(cam.origin, vec3_add(vec3_add(vec3_mult(cam.u, halfWidth * focusDist), vec3_mult(cam.v, halfHeight * focusDist)), vec3_mult(cam.w, focusDist)));
    cam.horizontal = vec3_mult(cam.u, 2.0 * halfWidth * focusDist);
    cam.vertical = vec3_mult(cam.v, 2.0 * halfHeight * focusDist);
    return cam;
}

Ray3D cam3D_ray(const Cam3D* restrict cam, const float s, const float t)
{
    vec2 r = vec2_rand();
    vec3 rand = vec3_new(r.x, r.y, 0.0);
    vec3 rd = vec3_mult(rand, cam->lensRadius);
    vec3 offset = vec3_add(vec3_mult(cam->u, rd.x), vec3_mult(cam->v, rd.y));
    vec3 p = _vec3_add(cam->origin, offset);
    return ray3D_new(p, vec3_normal(vec3_sub(vec3_add(cam->lowerLeftCorner, vec3_add(vec3_mult(cam->horizontal, s), vec3_mult(cam->vertical, t))), p)));
}
