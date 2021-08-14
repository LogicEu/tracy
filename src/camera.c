#include <tracy.h>

Camera camera_new(vec3 lookFrom, vec3 lookAt, vec3 vup, float vfov, float aspect, float aperture, float focusDist)
{
    Camera cam;
    cam.lensRadius = aperture / 2;
    float theta = vfov * M_PI/180;
    float halfHeight = tanf(theta/2);
    float halfWidth = aspect * halfHeight;
    cam.origin = lookFrom;
    cam.w = vec3_normal(vec3_sub(lookFrom, lookAt));
    cam.u = vec3_normal(vec3_cross(vup, cam.w));
    cam.v = vec3_cross(cam.w, cam.u);
    cam.lowerLeftCorner = vec3_sub(cam.origin, vec3_add(vec3_add(vec3_mult(cam.u, halfWidth*focusDist), vec3_mult(cam.v, halfHeight*focusDist)), vec3_mult(cam.w, focusDist)));
    cam.horizontal = vec3_mult(cam.u, 2 * halfWidth * focusDist);
    cam.vertical = vec3_mult(cam.v, 2 * halfHeight * focusDist);
    return cam;
}

Ray camera_ray(Camera* cam, float s, float t)
{
    vec3 rd = vec3_mult(random_in_disk(), cam->lensRadius);
    vec3 offset = vec3_add(vec3_mult(cam->u, rd.x), vec3_mult(cam->v, rd.y));
    vec3 p = vec3_add(cam->origin, offset);
    return ray_new(p, vec3_normal(vec3_sub(vec3_add(cam->lowerLeftCorner, vec3_add(vec3_mult(cam->horizontal, s), vec3_mult(cam->vertical, t))), p)));
}