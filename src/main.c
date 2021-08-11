#include <tracy.h>
#include <imgtool.h>
#include <stdlib.h>
#include <string.h>

#define DO_SAMPLES_PER_PIXEL 10
#define DO_ANIMATE 0
#define DO_ANIMATE_SMOOTHING 0.0f
#define DO_LIGHT_SAMPLING 100

static Sphere s_Spheres[] = {
    {{0, -100.5, -1}, 100, 0.0f},
    {{2, 0, -1}, 0.5f, 0.0f},
    {{0, 0, -1}, 0.5f, 0.0f},
    {{-2, 0, -1}, 0.5f, 0.0f},
    {{2, 0, 1}, 0.5f, 0.0f},
    {{0, 0, 1}, 0.5f, 0.0f},
    {{-2, 0, 1}, 0.5f, 0.0f},
    {{0.5f, 1, 0.5f}, 0.5f, 0.0f},
    {{-1.5f, 3.5f, 0.f}, 0.3f, 0.0f}
};

const int kSphereCount = sizeof(s_Spheres) / sizeof(s_Spheres[0]);

static Material s_SphereMats[] = {
    { Lambert, {0.8f, 0.8f, 0.8f}, {0, 0, 0}, 0, 0 },
    { Lambert, {0.8f, 0.4f, 0.4f}, {0, 0, 0}, 0, 0 },
    { Lambert, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0, 0 },
    { Metal, {0.4f, 0.4f, 0.8f}, {0, 0, 0}, 0, 0 },
    { Metal, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0, 0 },
    { Metal, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0.2f, 0 },
    { Metal, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0.6f, 0 },
    { Dielectric, {0.4f, 0.4f, 0.4f}, {0, 0, 0}, 0, 1.5f },
    { Lambert, {0.8f, 0.6f, 0.2f}, {100, 50, 30}, 0, 0 }
};

const float kMinT = 0.001f;
const float kMaxT = 1.0e7f;
const int kMaxDepth = 10;

float schlick(float cosine, float ri)
{
    float r0 = (1.0f - ri) / (1.0f + ri);
    r0 = r0*r0;
    return r0 + (1.0f - r0) * powf(1.0 - cosine, 5.0);
}

bool HitWorld(Ray* r, float tMin, float tMax, Hit* outHit, int* outID)
{
    Hit tmpHit;
    bool anything = false;
    float closest = tMax;
    for (int i = 0; i < kSphereCount; ++i)
    {
        if (HitSphere(r, &s_Spheres[i], tMin, closest, &tmpHit))
        {
            anything = true;
            closest = tmpHit.t;
            *outHit = tmpHit;
            *outID = i;
        }
    }
    return anything;
}


static bool Scatter(Material* mat, Ray* r_in, Hit* rec, vec3* attenuation, Ray* scattered, vec3* outLightE, int* inoutRayCount)
{
    *outLightE = vec3_uni(0.0f);
    if (mat->type == Lambert)
    {
        // random point inside unit sphere that is tangent to the hit point
        vec3 target = vec3_add(rec->pos, vec3_add(rec->normal, RandomInUnitSphere()));
        *scattered = ray_new(rec->pos, vec3_norm(vec3_sub(target, rec->pos)));
        *attenuation = mat->albedo;
        
        // sample lights
#if DO_LIGHT_SAMPLING
        for (int i = 0; i < kSphereCount; ++i)
        {
            Material* smat = &s_SphereMats[i];
            if (smat->emissive.x <= 0.0 && smat->emissive.y <= 0.0 && smat->emissive.z <= 0.0) continue; // skip non-emissive
            if (mat == smat) continue; // skip self
            Sphere* s = &s_Spheres[i];
            
            // create a random direction towards sphere
            // coord system for sampling: sw, su, sv
            vec3 sw = vec3_norm(vec3_sub(s->center, rec->pos));
            vec3 su = vec3_norm(vec3_cross(fabs(sw.x)>0.01f ? vec3_new(0.0, 1.0, 0.0) : vec3_new(1.0, 0.0, 0.0), sw));
            vec3 sv = vec3_cross(sw, su);
            // sample sphere by solid angle
            float cosAMax = sqrtf(1.0f - s->radius*s->radius / vec3_sqmag(vec3_sub(rec->pos, s->center)));
            float eps1 = randf_norm(), eps2 = randf_norm();
            float cosA = 1.0f - eps1 + eps1 * cosAMax;
            float sinA = sqrtf(1.0f - cosA*cosA);
            float phi = 2 * M_PI * eps2;
            vec3 l = vec3_add(vec3_mult(su, cosf(phi) * sinA), vec3_add(vec3_mult(sv, sin(phi) * sinA), vec3_mult(sw, cosA)));
            l = vec3_norm(l);
            
            // shoot shadow ray
            Hit lightHit;
            int hitID;
            Ray r = ray_new(rec->pos, l);
            (*inoutRayCount)++;
            if (HitWorld(&r, kMinT, kMaxT, &lightHit, &hitID) && hitID == i)
            {
                float omega = 2 * M_PI * (1-cosAMax);
                
                vec3 rdir = r_in->dir;
                AssertUnit(rdir);
                vec3 nl = vec3_dot(rec->normal, rdir) < 0 ? rec->normal : vec3_neg(rec->normal);
                *outLightE = vec3_add(*outLightE, vec3_mult(vec3_prod(mat->albedo, smat->emissive), maxf(0.0f, vec3_dot(l, nl)) * omega / M_PI));
            }
        }
#endif
        return true;
    }
    else if (mat->type == Metal)
    {
        AssertUnit(r_in->dir); AssertUnit(rec->normal);
        vec3 refl = vec3_reflect(r_in->dir, rec->normal);
        // reflected ray, and random inside of sphere based on roughness
        *scattered = ray_new(rec->pos, vec3_norm(vec3_add(refl, vec3_mult(RandomInUnitSphere(), mat->roughness))));
        *attenuation = mat->albedo;
        return vec3_dot(scattered->dir, rec->normal) > 0.0f;
    }
    else if (mat->type == Dielectric)
    {
        AssertUnit(r_in->dir); 
        AssertUnit(rec->normal);
        vec3 outwardN;
        vec3 rdir = r_in->dir;
        vec3 refl = vec3_reflect(rdir, rec->normal);
        float nint;
        *attenuation = vec3_uni(1.0);
        vec3 refr;
        float reflProb;
        float cosine;
        if (vec3_dot(rdir, rec->normal) > 0)
        {
            outwardN = vec3_neg(rec->normal);
            nint = mat->ri;
            cosine = mat->ri * vec3_dot(rdir, rec->normal);
        }
        else
        {
            outwardN = rec->normal;
            nint = 1.0f / mat->ri;
            cosine = -vec3_dot(rdir, rec->normal);
        }
        if (vec3_refract(rdir, outwardN, nint, &refr))
        {
            reflProb = schlick(cosine, mat->ri);
        }
        else
        {
            reflProb = 1;
        }
        if (randf_norm() < reflProb)
            *scattered = ray_new(rec->pos, vec3_norm(refl));
        else
            *scattered = ray_new(rec->pos, vec3_norm(refr));
    }
    else
    {
        *attenuation = vec3_new(1.0, 0.0, 1.0);
        return false;
    }
    return true;
}

static vec3 Trace(Ray* r, int depth, int* inoutRayCount)
{
    Hit rec;
    int id = 0;
    (*inoutRayCount)++;
    if (HitWorld(r, kMinT, kMaxT, &rec, &id))
    {
        Ray scattered;
        vec3 attenuation;
        vec3 lightE;
        Material* mat = &s_SphereMats[id];
        if (depth < kMaxDepth && Scatter(mat, r, &rec, &attenuation, &scattered, &lightE, inoutRayCount))
        {
            return vec3_add(mat->emissive, vec3_add(lightE, vec3_prod(attenuation, Trace(&scattered, depth + 1, inoutRayCount))));
        }
        else
        {
            return mat->emissive;
        }
    }
    else
    {
        // sky
        vec3 unitDir = r->dir;
        float t = 0.5f * (unitDir.y + 1.0f);
        return vec3_mult(vec3_add(vec3_mult(vec3_new(1.0f, 1.0f, 1.0f), 1.0f - t), vec3_mult(vec3_new(0.5f, 0.7f, 1.0f), t)), 0.3f);
    }
}

typedef struct JobData {
    float time;
    int frameCount;
    int screenWidth, screenHeight;
    float* backbuffer;
    Camera* cam;
    int rayCount;
} JobData;

static void TraceRowJob(uint32_t start, uint32_t end, uint32_t threadnum, void* data_)
{
    JobData data = *(JobData*)data_;
    float* backbuffer = data.backbuffer + start * data.screenWidth * 4;
    float invWidth = 1.0f / data.screenWidth;
    float invHeight = 1.0f / data.screenHeight;
    float lerpFac = (float)data.frameCount / (float)(data.frameCount+1);
#if DO_ANIMATE
    lerpFac *= DO_ANIMATE_SMOOTHING;
#endif
    int rayCount = 0;
    for (uint32_t y = start; y < end; ++y)
    {
        for (int x = 0; x < data.screenWidth; ++x)
        {
            vec3 col = vec3_uni(0.0f);
            for (int s = 0; s < DO_SAMPLES_PER_PIXEL; s++)
            {
                float u = ((float)x + randf_norm()) * invWidth;
                float v = ((float)y + randf_norm()) * invHeight;
                Ray r = GetRay(data.cam, u, v);
                col = vec3_add(col, Trace(&r, 0, &rayCount));
            }
            col = vec3_mult(col, 1.0f / (float)DO_SAMPLES_PER_PIXEL);
            col = vec3_new(sqrtf(col.x), sqrtf(col.y), sqrtf(col.z));
            
            vec3 prev = vec3_new(backbuffer[0], backbuffer[1], backbuffer[2]);
            col = vec3_add(vec3_mult(prev, lerpFac), vec3_mult(col, (1.0f - lerpFac)));
            backbuffer[0] = col.x;
            backbuffer[1] = col.y;
            backbuffer[2] = col.z;
            backbuffer[3] = 1.0f;
            backbuffer += 4;
        }
    }
    data.rayCount += rayCount;
}

void DrawTest(float time, int frameCount, int screenWidth, int screenHeight, float* backbuffer, int* outRayCount)
{
#if DO_ANIMATE
    s_Spheres[1].center.y = cosf(time)+1.0f;
    s_Spheres[8].center.z = sinf(time)*0.3f;
#endif
    static vec3 lookfrom = {0.0, 2.0, 3.0};
    static vec3 lookat = {0.0, 0.0, 0.0};
    float distToFocus = 3.0f;
    float aperture = 0.1f;
    
    for (int i = 0; i < kSphereCount; ++i) {
        sphereUpdateDerivedData(&s_Spheres[i]);
    }
    
    Camera cam = camera_new(lookfrom, lookat, vec3_new(0.0, 1.0, 0.0), 60, (float)screenWidth / (float)screenHeight, aperture, distToFocus);

    JobData args;
    args.time = time;
    args.frameCount = frameCount;
    args.screenWidth = screenWidth;
    args.screenHeight = screenHeight;
    args.backbuffer = backbuffer;
    args.cam = &cam;
    args.rayCount = 0;

    for (uint32_t i = 0; i < screenHeight; i++) {
        uint32_t start = i, end = i + 1;
        TraceRowJob(start, end, 1, &args);
    }

    *outRayCount = args.rayCount;

    lookfrom.z += 0.04f;
    s_Spheres[7].center.x -= 0.08;
}

int main(int argc, char** argv) 
{   
    int ray_count = 0, iters = 10;
    uint32_t width = 400, height = 400;
    char path[128] = "out.gif", op[128];

    if (argc > 1) width = (uint32_t)atoi(argv[1]);
    if (argc > 2) height = (uint32_t)atoi(argv[2]);
    float* backbuffer = (float*)malloc(sizeof(float) * width * height * 4);
    printf("Rendering path traced scene...\nwidth: %d\nheight: %d\n", width, height);

    bmp_t bmps[iters];
    for (int i = 0; i < iters; i++) {
        int ray = 0;
        printf("Rendering frame %d of %d\n", i + 1, iters);
        DrawTest(0.0, 1, width, height, backbuffer, &ray);
        ray_count += ray;
        bmp_t bmp = bmp_new(width, height, 4);
        for (int j = 0; j < width * height * 4; j++) {
            bmp.pixels[j] = (uint8_t)(backbuffer[j] * 255.0f);
        }
        bmp_t tmp = bmp_flip_vertical(&bmp);
        bmps[i] = bmp_transform(&tmp, 3);
        bmp_free(&tmp);
        bmp_free(&bmp);
    }

    gif_t* gif = bmp_to_gif(&bmps[0], iters);
    gif_file_write(path, gif);
    //gif_free(gif);
    free(backbuffer);

#ifdef __APPLE__
    strcpy(op, "open ");
#else 
    strcpy(op, "xdg-open ");
#endif
    strcat(op, path);
    system(op);
    return EXIT_SUCCESS;
}
