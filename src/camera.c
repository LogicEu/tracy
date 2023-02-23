#include <tracy.h>

Cam3D cam3D_new(const vec3 lookFrom, const vec3 lookAt, const vec3 up, const float fov, const float aspect, const float aperture, const float focusDist)
{
    Cam3D cam;
    cam.lookFrom = lookFrom;
    cam.lookAt = lookAt;
    cam.up = up;
    cam.fov = fov;
    cam.aspect = aspect;
    cam.aperture = aperture;
    cam.focusDist = focusDist;
    cam3D_update(&cam);
    return cam;
}

void cam3D_update(Cam3D* restrict cam)
{
    const float halfHeight = tanf(cam->fov * M_PI / 360.0);
    const float halfWidth = cam->aspect * halfHeight;
    cam->params.w = vec3_normal(_vec3_sub(cam->lookFrom, cam->lookAt));
    cam->params.u = vec3_normal(_vec3_cross(cam->up, cam->params.w));
    cam->params.v = _vec3_cross(cam->params.w, cam->params.u);
    cam->params.lowerLeftCorner = vec3_sub(cam->lookFrom, vec3_add(vec3_add(_vec3_mult(cam->params.u, halfWidth * cam->focusDist), _vec3_mult(cam->params.v, halfHeight * cam->focusDist)), _vec3_mult(cam->params.w, cam->focusDist)));
    cam->params.horizontal = _vec3_mult(cam->params.u, 2.0 * halfWidth * cam->focusDist);
    cam->params.vertical = _vec3_mult(cam->params.v, 2.0 * halfHeight * cam->focusDist);
}

Ray3D cam3D_ray(const Cam3D* restrict cam, const float s, const float t)
{
    const float k = cam->aperture * 0.5;
    vec2 rd = {frand_signed() * k, frand_signed() * k};
    vec3 offset = vec3_add(vec3_mult(cam->params.u, rd.x), _vec3_mult(cam->params.v, rd.y));
    vec3 p = _vec3_add(cam->lookFrom, offset);
    return ray3D_new(p, vec3_normal(vec3_sub(vec3_add(cam->params.lowerLeftCorner, vec3_add(_vec3_mult(cam->params.horizontal, s), _vec3_mult(cam->params.vertical, t))), p)));
}
