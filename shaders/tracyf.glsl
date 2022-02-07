out vec4 FragColor;

in vec4 spheres;

uniform float u_time;
uniform vec2 u_resolution;
uniform vec3 u_position;
uniform vec3 u_lookat;

#define PI 3.14159265358979323846
#define PHI 1.61803398874989484820459

#define MAT_LAMBERT 0
#define MAT_METAL 1
#define MAT_DIELECTRIC 2

const float kMinT = 0.001;
const float kMaxT = 1.0e7;
const int kMaxDepth = 10;

struct Ray3D {
    vec3 orig;
    vec3 dir;
};

struct Hit3D {
    vec3 normal;
    float t;
};

struct Cam3D {
    vec3 origin;
    vec3 lowerLeftCorner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    float lensRadius;
};

struct Material {
    int type;
    vec3 albedo;
    vec3 emissive;
    float roughness;
    float ri;
};

vec4 g_sphere = vec4(0.0, 0.0, 4.0, 1.0);
Material g_material = Material(
    0,
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 0.0, 0.0),
    0.1,
    0.0
);

float rand(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233)) * u_time) * 43758.5453);
}

Cam3D camera_new(vec3 lookfrom, vec3 lookat, vec3 vup, float fov, float aspect, float aperture, float focusDist)
{
    Cam3D cam;
    cam.lensRadius = aperture * 0.5;
    float theta = fov * PI / 180.0;
    float halfHeight = tan(theta * 0.5);
    float halfWidth = aspect * halfHeight;
    cam.origin = lookfrom;
    cam.w = normalize(lookfrom - lookat);
    cam.u = normalize(cross(vup, cam.w));
    cam.v = cross(cam.w, cam.u);
    cam.lowerLeftCorner = cam.origin - cam.u * halfWidth * focusDist + cam.v * halfHeight * focusDist + cam.w * focusDist;
    cam.horizontal = cam.u * 2.0 * halfWidth * focusDist;
    cam.vertical = cam.v * 2.0 * halfHeight * focusDist;
    return cam;
}

Ray3D camera_ray(Cam3D cam, float s, float t)
{
    vec3 rand = vec3(vec2(rand(vec2(s, t)), rand(vec2(s, t))), 0.0);
    vec3 rd = rand * cam.lensRadius;
    vec3 offset = cam.u * rd.x + cam.v * rd.y;
    vec3 p = cam.origin + offset;
    return Ray3D(p, normalize(cam.lowerLeftCorner + cam.horizontal * s + cam.vertical * t - p));
}

bool sphere_hit(vec4 sphere, Ray3D ray, inout Hit3D outHit)
{
    vec3 oc = ray.orig - sphere.xyz;
    float b = dot(oc, ray.dir);
    float c = dot(oc, oc) - sphere.w * sphere.w;
    float discr = b * b - c;

    if (discr > 0.0f) {
        float t = (-b - sqrt(discr));
        vec3 p = ray.orig + ray.dir * t;
        outHit.t = t;
        outHit.normal = (p - sphere.xyz) * (1.0f / sphere.w);
        return true;
    }
    return false;
}

bool scene_hit(Ray3D ray, inout Hit3D outHit, inout int id, float fmin, float fmax)
{
    Hit3D tmpHit;
    float closest = fmax;
    bool anything = false;

    /*Tri3D* tri = triangles->data;
    for (unsigned int i = 0; i < triangles->used; i++) {
        if (tri3D_hit(tri++, ray, &tmpHit) && tmpHit.t > tMin && tmpHit.t < closest) {
            closest = tmpHit.t;
            *outHit = tmpHit;
            *outID = *(int*)array_index(trimaterials, i);
            anything = true;
        }
    }*/

    //for (unsigned int i = 0; i < spheres->used; ++i) {
    vec4 sphere = g_sphere;
    if (sphere_hit(sphere, ray, tmpHit) && (tmpHit.t > fmin) && (tmpHit.t < closest)) {
        closest = tmpHit.t;
        outHit = tmpHit;
        //id = i;
        anything = true;
    }
    return anything;
}

bool ray_scatter(Material mat, Ray3D ray, inout Hit3D rec, inout vec3 attenuation, inout Ray3D scattered, inout vec3 outLightE)
{
    outLightE = vec3(0.0);
    vec3 P = ray.orig + ray.dir * rec.t;
    if (mat.type == MAT_LAMBERT) {
        // random point inside unit sphere that is tangent to the hit point
        vec3 target = P + rec.normal + vec3(rand(ray.dir.xy), rand(ray.dir.xy), rand(ray.dir.xy));
        scattered = Ray3D(P, normalize(target - P));
        attenuation = mat.albedo;
        
        // sample lights
        //for (int i = 0; i < (int)spheres->used; ++i) {
        /*Material smat = array_index(materials, i);
        if (smat->emissive.x <= 0.0 && smat->emissive.y <= 0.0 && smat->emissive.z <= 0.0) continue; // skip non-emissive
        if (mat == smat) continue; // skip self
        
        // create a random direction towards sphere
        // coord system for sampling: sw, su, sv
        vec3 sw = vec3_normal(vec3_sub(s->pos, P));
        vec3 su = vec3_normal(vec3_cross(_absf(sw.x) > 0.01f ? vec3_new(0.0, 1.0, 0.0) : vec3_new(1.0, 0.0, 0.0), sw));
        vec3 sv = vec3_cross(sw, su);
        // sample sphere by solid angle
        float cosAMax = sqrtf(1.0f - s->radius * s->radius / vec3_sqmag(vec3_sub(P, s->pos)));
        float eps1 = randf_norm(), eps2 = randf_norm();
        float cosA = 1.0f - eps1 + eps1 * cosAMax;
        float sinA = sqrtf(1.0f - cosA * cosA);
        float phi = 2 * M_PI * eps2;
        vec3 l = vec3_add(vec3_mult(su, cosf(phi) * sinA), vec3_add(vec3_mult(sv, sin(phi) * sinA), vec3_mult(sw, cosA)));
        l = vec3_normal(l);
        
        // shoot shadow ray
        Hit3D lightHit;
        int hitID;
        Ray3D r = {P, l};
        (*inoutRayCount)++;
        if (scene_hit(&r, &lightHit, &hitID, kMinT, kMaxT) && hitID == i) {
            float omega = 2.0 * M_PI * (1.0 - cosAMax);
            
            vec3 nl = _vec3_dot(rec->normal, ray->dir) < 0 ? rec->normal : vec3_neg(rec->normal);
            *outLightE = vec3_add(*outLightE, vec3_mult(vec3_prod(mat->albedo, smat->emissive), _maxf(0.0f, _vec3_dot(l, nl)) * omega / M_PI));
        }*/
        //s++;
        //}
        return true;
    }
    return false;
}

vec3 ray_traceD(Ray3D ray, int depth)
{
    Hit3D rec;
    int id;

    if (scene_hit(ray, rec, id, kMinT, kMaxT)) {
        Ray3D scattered;
        vec3 attenuation = vec3(0.0);
        vec3 light = vec3(0.0);
        Material mat = g_material;

        if (depth < kMaxDepth && ray_scatter(mat, ray, rec, attenuation, scattered, light)) {
            return mat.emissive + light + attenuation;// * ray_traceD(scattered, depth + 1);
        } else return mat.emissive;
    } else {
        // Sky
        float t = clamp(ray.dir.y, 0.1, 1.0) * 0.3;
        return vec3(0.3, 0.3, 1.0) * t;
    }
}

vec3 ray_trace(Ray3D ray, int depth)
{
    Hit3D rec;
    int id;

    if (scene_hit(ray, rec, id, kMinT, kMaxT)) {
        return g_material.albedo * rec.normal;
        /*Ray3D scattered;
        vec3 attenuation = vec3(0.0);
        vec3 light = vec3(0.0);
        Material mat = g_material;

        if (depth < kMaxDepth && ray_scatter(mat, ray, rec, attenuation, scattered, light)) {
            return mat.emissive + light + attenuation * ray_traceD(scattered, depth + 1);
        } else return mat.emissive;*/
    } else {
        // Sky
        float t = clamp(ray.dir.y, 0.1, 1.0) * 0.3;
        return vec3(0.3, 0.3, 1.0) * t;
    }
}

vec3 path_tracy(Ray3D ray)
{
    vec4 sphere = g_sphere;
    vec3 col = vec3(0.0);
    vec3 oc = ray.orig - sphere.xyz;
    float b = dot(oc, ray.dir);
    float c = dot(oc, oc) - sphere.w * sphere.w;
    float discr = b * b - c;
    float n = float(discr > 0.0);
    col.r = n;
    return col;
}

void main()
{   
    vec2 uv = (gl_FragCoord.xy - u_resolution) / u_resolution.y;

    float res = u_resolution.x / u_resolution.y;
    vec3 up = vec3(0.0, 1.0, 0.0);

    Cam3D cam = camera_new(u_position, u_lookat, up, 70.0, res, 0.1, 8.0);
    Ray3D ray = camera_ray(cam, uv.x, uv.y);
    vec3 c = ray_trace(ray, 1);

    FragColor = vec4(c, 1.0);
}