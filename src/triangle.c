#include <tracy.h>

// https://stackoverflow.com/questions/31178874/how-to-get-the-intersection-point-ray-triangle-intersection-c

vec3 triangle_norm(Triangle* tri)
{
    vec3 e1 = vec3_sub(tri->b, tri->a);
    vec3 e2 = vec3_sub(tri->c, tri->a);
    return vec3_norm(vec3_cross(e1, e2)); 
}

void traingle_normalize(Triangle* tri)
{
    tri->n = triangle_norm(tri); 
}

bool triangle_hit(Ray* ray, Triangle tri, float tMin, float tMax, Hit* outHit)
{ 
    float nRayDir = vec3_dot(tri.n, ray->dir);
    if (fabs(nRayDir) < 0.01) return false; // They are parallel!

    // compute d parameter using equation 2
    float d = vec3_dot(tri.n, tri.a); 

    // compute t (equation P=O+tR P intersection point ray origin O and its direction R)

    float t = -(vec3_dot(tri.n, ray->orig) - d) / nRayDir; 

    if (t < tMin || t > tMax) return false; // the triangle is on another depth 

    // compute the intersection point using equation
    vec3 P; 

    //this part should do the work, but it does not work.
    P = vec3_add(ray->orig, vec3_mult(ray->dir, t));
    outHit->pos = P;
    outHit->t = t;
    outHit->normal = tri.n;

    // Step 2: inside-outside test
    vec3 C; // vector perpendicular to triangle's plane 

    // edge 0
    vec3 edge0 = vec3_sub(tri.b, tri.a);
    vec3 vp0 = vec3_sub(P, tri.a);

    C = vec3_cross(edge0, vp0); 
    if (vec3_dot(tri.n, C) < 0) return false; // P is on the right side 


    vec3 edge1 = vec3_sub(tri.c, tri.b);
    vec3 vp1 = vec3_sub(P, tri.b);

    C = vec3_cross(edge1, vp1); 

    if (vec3_dot(tri.n, C) < 0) return false; // P is on the right side 

    // edge 2
    vec3 edge2 = vec3_sub(tri.a, tri.c);
    vec3 vp2 = vec3_sub(P, tri.c); 

    C = vec3_cross(edge2, vp2);

    if (vec3_dot(tri.n, C) < 0) return false; // P is on the right side; 

    return true; // this ray hits the triangle 
} 