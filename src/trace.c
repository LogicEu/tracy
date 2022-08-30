#include <tracy.h>

static inline float schlick(const float cos, float ri)
{
    ri = (1.0f - ri) / (1.0f + ri);
    ri = ri * ri;
    return ri + (1.0f - ri) * powf(1.0 - cos, 5.0);
}

static bool ray3D_scatter(const Scene3D* scene, const Material* restrict mat, const Ray3D* restrict ray, Hit3D* restrict rec, vec3* attenuation, Ray3D* restrict scattered, vec3* restrict outLight)
{
    *outLight = (vec3){0.0, 0.0, 0.0};
    const vec3 P = _ray3D_at(ray, rec->t);
    
    if (mat->type == Lambert) {
        
        // random point inside unit sphere that is tangent to the hit point
        const vec3 target = vec3_add(P, vec3_add(rec->normal, vec3_rand()));
        *scattered = ray3D_new(P, vec3_normal(_vec3_sub(target, P)));
        *attenuation = mat->albedo;

        // sample lights
        const size_t* indices = scene->sphere_materials.data;
        const Material* materials = scene->materials.data;
        const Sphere* s = scene->spheres.data;
        const size_t sphere_count = scene->spheres.size;
        for (size_t i = 0; i < sphere_count; ++i, ++s) {
            
            const size_t n = indices[i];
            const Material* smat = materials + n;
            
            if ((smat->emissive.x <= 0.0 && smat->emissive.y <= 0.0 && 
                smat->emissive.z <= 0.0) || mat == smat) {
                continue;
            }
            
            // create a random direction towards sphere
            // coord system for sampling: sw, su, sv
            vec3 sw = vec3_normal(_vec3_sub(s->pos, P));
            vec3 su = vec3_normal(vec3_cross(_absf(sw.x) > 0.01f ? _vec3_new(0.0, 1.0, 0.0) : _vec3_new(1.0, 0.0, 0.0), sw));
            vec3 sv = _vec3_cross(sw, su);
            // sample sphere by solid angle
            float cosAMax = sqrtf(1.0f - s->radius * s->radius / vec3_sqmag(_vec3_sub(P, s->pos)));
            float eps1 = randf_norm(), eps2 = randf_norm();
            float cosA = 1.0f - eps1 + eps1 * cosAMax;
            float sinA = sqrtf(1.0f - cosA * cosA);
            float phi = 2.0 * M_PI * eps2;
            vec3 l = vec3_add(_vec3_mult(su, cosf(phi) * sinA), vec3_add(_vec3_mult(sv, sin(phi) * sinA), _vec3_mult(sw, cosA)));
            l = vec3_normal(l);
            
            // shoot shadow ray
            Hit3D lightHit;
            size_t hitID;
            Ray3D r = {P, l};

            if (scene3D_hit(scene, &r, &lightHit, &hitID) && hitID == n) {
                float omega = 2.0 * (1.0 - cosAMax);
                vec3 nl = _vec3_dot(rec->normal, ray->dir) < 0 ? rec->normal : _vec3_neg(rec->normal);
                *outLight = vec3_add(*outLight, vec3_mult(vec3_prod(mat->albedo, smat->emissive), _maxf(0.0f, _vec3_dot(l, nl)) * omega));
            }
        }
    }
    else if (mat->type == Metal) {
        vec3 refl = vec3_reflect(ray->dir, rec->normal);
        // reflected ray, and random inside of sphere based on roughness
        *scattered = ray3D_new(P, vec3_normal(vec3_add(refl, vec3_mult(vec3_rand(), mat->roughness))));
        *attenuation = mat->albedo;
        return _vec3_dot(scattered->dir, rec->normal) > 0.0f;
    }
    else if (mat->type == Dielectric) {
        vec3 outwardN;
        vec3 refl = vec3_reflect(ray->dir, rec->normal);
        float nint;
        *attenuation = _vec3_uni(1.0f);
        vec3 refr;
        float reflProb;
        float cosine;
        if (_vec3_dot(ray->dir, rec->normal) > 0.0f) {
            outwardN = _vec3_neg(rec->normal);
            nint = mat->ri;
            cosine = mat->ri * _vec3_dot(ray->dir, rec->normal);
        } else {
            outwardN = rec->normal;
            nint = 1.0f / mat->ri;
            cosine = -_vec3_dot(ray->dir, rec->normal);
        }
        
        if (vec3_refract(ray->dir, outwardN, nint, &refr)) {
            reflProb = schlick(cosine, mat->ri);
        } else reflProb = 1.0f;
        
        if (randf_norm() < reflProb) *scattered = ray3D_new(P, vec3_normal(refl));
        else *scattered = ray3D_new(P, vec3_normal(refr));
    }
    else return false;

    return true;
}

vec3 ray3D_trace(const Scene3D* restrict scene, const Ray3D* restrict ray, const uint32_t depth)
{
    Hit3D rec;
    size_t id;

    if (scene3D_hit(scene, ray, &rec, &id)) {
        Ray3D scattered;
        vec3 attenuation, light;
        Material* mat = (Material*)scene->materials.data + id;
        if (depth < TRACY_MAX_DEPTH && ray3D_scatter(scene, mat, ray, &rec, &attenuation, &scattered, &light)) {
            return vec3_add(mat->emissive, vec3_add(light, vec3_prod(attenuation, ray3D_trace(scene, &scattered, depth + 1))));
        } else return mat->emissive;
    } else {
        // Sky
        float t = clampf(ray->dir.y, 0.1, 1.0);
        return _vec3_mult(scene->background_color, t);
    }
}
