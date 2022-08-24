#include <tracy.h>

Cam3D cam3D_new(const vec3 lookFrom, const vec3 lookAt, const vec3 up, const float fov, const float aspect, const float aperture, const float focusDist)
{
    Cam3D cam;
    cam.params.lookFrom = lookFrom;
    cam.params.lookAt = lookAt;
    cam.params.up = up;
    cam.params.fov = fov;
    cam.params.aspect = aspect;
    cam.params.aperture = aperture;
    cam.params.focusDist = focusDist;
    cam3D_update(&cam);
    return cam;
}

void cam3D_update(Cam3D* restrict cam)
{
    float theta = cam->params.fov * M_PI / 180.0;
    float halfHeight = tanf(theta * 0.5);
    float halfWidth = cam->params.aspect * halfHeight;
    cam->w = vec3_normal(vec3_sub(cam->params.lookFrom, cam->params.lookAt));
    cam->u = vec3_normal(vec3_cross(cam->params.up, cam->w));
    cam->v = vec3_cross(cam->w, cam->u);
    cam->lowerLeftCorner = vec3_sub(cam->params.lookFrom, vec3_add(vec3_add(vec3_mult(cam->u, halfWidth * cam->params.focusDist), vec3_mult(cam->v, halfHeight * cam->params.focusDist)), vec3_mult(cam->w, cam->params.focusDist)));
    cam->horizontal = vec3_mult(cam->u, 2.0 * halfWidth * cam->params.focusDist);
    cam->vertical = vec3_mult(cam->v, 2.0 * halfHeight * cam->params.focusDist);
}

Ray3D cam3D_ray(const Cam3D* restrict cam, const float s, const float t)
{
    vec2 r = vec2_rand();
    vec3 rand = vec3_new(r.x, r.y, 0.0);
    vec3 rd = vec3_mult(rand, cam->params.aperture * 0.5);
    vec3 offset = vec3_add(vec3_mult(cam->u, rd.x), vec3_mult(cam->v, rd.y));
    vec3 p = _vec3_add(cam->params.lookFrom, offset);
    return ray3D_new(p, vec3_normal(vec3_sub(vec3_add(cam->lowerLeftCorner, vec3_add(vec3_mult(cam->horizontal, s), vec3_mult(cam->vertical, t))), p)));
}
