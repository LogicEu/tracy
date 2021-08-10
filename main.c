#include <imgtool.h>
#include <libfract.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define kPI 3.1415926

float vec3_sqdist(vec3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float _fast_inverse_sqrt(float num) // Viva Quake forever!
{
	float x = num * 0.5;
	float y  = num;
	long i  = *(long*)&y;
	i  = 0x5f3759df - (i >> 1);
	y  = *(float *)&i;
	y  = y * (1.5 - (x * y * y));
    y = y * (1.5 - (x * y * y));
	return y;
}

vec3 vec3_norml(vec3 v)
{
    float i = _fast_inverse_sqrt(vec3_dot(v, v));
    //float k = 1.0f / sqrtf(vec3_sqdist(v)); 
    //printf("Fast: %f Slow: %f\n", i, k);
    return vec3_new(v.x * i, v.y * i, v.z * i);
}

inline void AssertUnit(vec3 v)
{
    assert(fabsf(vec3_sqdist(v) - 1.0f) < 0.1f);
}

#define AssertUnit(v)                               \
do {                                                \
    if (!(fabsf(vec3_sqdist(v) - 1.0f) < 0.01f)) {  \
        vec3_print(v);                              \
        printf("%d, %s\n", __LINE__, __func__);     \
    }                                               \
} while (0)

vec3 vec3_reflect(vec3 v, vec3 n)
{
    return vec3_sub(v, vec3_mult(n, 2.0f * vec3_dot(v, n)));
}

bool vec3_refract(vec3 v, vec3 n, float nint, vec3* outRefracted)
{
    AssertUnit(v);
    float dt = vec3_dot(v, n);
    float discr = 1.0f - nint * nint * (1.0f - dt * dt);
    if (discr > 0) {
        vec3 tmp = vec3_mult(vec3_sub(v, vec3_mult(n, dt)), nint);
        *outRefracted = vec3_sub(tmp, vec3_mult(n, sqrtf(discr)));
        return true;
    }
    return false;
}

float schlick(float cosine, float ri)
{
    float r0 = (1.0f - ri) / (1.0f + ri);
    r0 = r0*r0;
    return r0 + (1.0f - r0) * powf(1.0 - cosine, 5.0);
}

typedef struct Ray {
    vec3 orig;
    vec3 dir;
} Ray;

Ray ray_new(vec3 orig, vec3 dir) 
{
    AssertUnit(dir);
    Ray r = {orig, dir};
    return r;
}

vec3 pointAt(Ray* r, float t) 
{ 
    return vec3_add(r->orig, vec3_mult(r->dir, t)); 
}

typedef struct Hit {
    vec3 pos;
    vec3 normal;
    float t;
} Hit;

typedef struct Sphere
{   
    vec3 center;
    float radius;
    float invRadius;
} Sphere;

void sphereUpdateDerivedData(Sphere* s) 
{ 
    s->invRadius = 1.0f / s->radius; 
}

bool HitSphere(Ray* r, Sphere* s, float tMin, float tMax, Hit* outHit);

float RandomFloat01();
vec3 RandomInUnitDisk();
vec3 RandomInUnitSphere();

vec3 vec3_prod(vec3 a, vec3 b)
{
    vec3 ret = {a.x * b.x, a.y * b.y, a.z * b.z};
    return ret;
}

typedef struct Camera {
    vec3 origin;
    vec3 lowerLeftCorner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    float lensRadius;
} Camera;

Camera camera_new(vec3 lookFrom, vec3 lookAt, vec3 vup, float vfov, float aspect, float aperture, float focusDist)
{
    Camera cam;
    cam.lensRadius = aperture / 2;
    float theta = vfov * M_PI/180;
    float halfHeight = tanf(theta/2);
    float halfWidth = aspect * halfHeight;
    cam.origin = lookFrom;
    cam.w = vec3_norml(vec3_sub(lookFrom, lookAt));
    cam.u = vec3_norml(vec3_cross(vup, cam.w));
    cam.v = vec3_cross(cam.w, cam.u);
    cam.lowerLeftCorner = vec3_sub(cam.origin, vec3_add(vec3_add(vec3_mult(cam.u, halfWidth*focusDist), vec3_mult(cam.v, halfHeight*focusDist)), vec3_mult(cam.w, focusDist)));
    cam.horizontal = vec3_mult(cam.u, 2 * halfWidth * focusDist);
    cam.vertical = vec3_mult(cam.v, 2 * halfHeight * focusDist);
    return cam;
}

Ray GetRay(Camera* cam, float s, float t)
{
    vec3 rd = vec3_mult(RandomInUnitDisk(), cam->lensRadius);
    vec3 offset = vec3_add(vec3_mult(cam->u, rd.x), vec3_mult(cam->v, rd.y));
    vec3 p = vec3_add(cam->origin, offset);
    return ray_new(p, vec3_norml(vec3_sub(vec3_add(cam->lowerLeftCorner, vec3_add(vec3_mult(cam->horizontal, s), vec3_mult(cam->vertical, t))), p)));
}

static uint32_t s_RndState = 1;

static uint32_t XorShift32()
{
    uint32_t x = s_RndState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 15;
    s_RndState = x;
    return x;
}

float RandomFloat01()
{
    return (XorShift32() & 0xFFFFFF) / 16777216.0f;
}

vec3 RandomInUnitDisk()
{
    vec3 p;
    do {
        p = vec3_sub(vec3_mult(vec3_new(RandomFloat01(), RandomFloat01(), 0.0), 2.0f), vec3_new(1.0, 1.0, 0.0));
    } while (vec3_dot(p, p) >= 1.0);
    return p;
}

vec3 RandomInUnitSphere()
{
    vec3 p;
    do {
        p = vec3_sub(vec3_mult(vec3_new(RandomFloat01(), RandomFloat01(), RandomFloat01()), 2.0f), vec3_new(1.0, 1.0, 1.0));
    } while (vec3_sqdist(p) >= 1.0);
    return p;
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

void InitializeTest();
void ShutdownTest();
void DrawTest(float time, int frameCount, int screenWidth, int screenHeight, float* backbuffer, int* outRayCount);

#define DO_SAMPLES_PER_PIXEL 100
#define DO_ANIMATE 0
#define DO_ANIMATE_SMOOTHING 0.0f
#define DO_LIGHT_SAMPLING 10

static Sphere s_Spheres[] = {
    {{0, -100.5, -1}, 100, 0.0f},
    {{2, 0, -1}, 0.5f, 0.0f},
    {{0, 0, -1}, 0.5f, 0.0f},
    {{-2, 0, -1}, 0.5f, 0.0f},
    {{2, 0, 1}, 0.5f, 0.0f},
    {{0, 0, 1}, 0.5f, 0.0f},
    {{-2, 0, 1}, 0.5f, 0.0f},
    {{0.5f, 1, 0.5f}, 0.5f, 0.0f},
    {{-1.5f, 1.5f, 0.f}, 0.3f, 0.0f}
};

const int kSphereCount = sizeof(s_Spheres) / sizeof(s_Spheres[0]);

typedef enum {
    Lambert, 
    Metal,
    Dielectric
} Type;

typedef struct Material {
    Type type;
    vec3 albedo;
    vec3 emissive;
    float roughness;
    float ri;
} Material;

static Material s_SphereMats[kSphereCount] = {
    { Lambert, {0.8f, 0.8f, 0.8f}, {0, 0, 0}, 0, 0 },
    { Lambert, {0.8f, 0.4f, 0.4f}, {0, 0, 0}, 0, 0 },
    { Lambert, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0, 0 },
    { Metal, {0.4f, 0.4f, 0.8f}, {0, 0, 0}, 0, 0 },
    { Metal, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0, 0 },
    { Metal, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0.2f, 0 },
    { Metal, {0.4f, 0.8f, 0.4f}, {0, 0, 0}, 0.6f, 0 },
    { Dielectric, {0.4f, 0.4f, 0.4f}, {0, 0, 0}, 0, 1.5f },
    { Lambert, {0.8f, 0.6f, 0.2f}, {30, 25, 15}, 0, 0 }
};

const float kMinT = 0.001f;
const float kMaxT = 1.0e7f;
const int kMaxDepth = 10;

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
        *scattered = ray_new(rec->pos, vec3_norml(vec3_sub(target, rec->pos)));
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
            vec3 sw = vec3_norml(vec3_sub(s->center, rec->pos));
            vec3 su = vec3_norml(vec3_cross(fabs(sw.x)>0.01f ? vec3_new(0.0, 1.0, 0.0) : vec3_new(1.0, 0.0, 0.0), sw));
            vec3 sv = vec3_cross(sw, su);
            // sample sphere by solid angle
            float cosAMax = sqrtf(1.0f - s->radius*s->radius / vec3_sqdist(vec3_sub(rec->pos, s->center)));
            float eps1 = RandomFloat01(), eps2 = RandomFloat01();
            float cosA = 1.0f - eps1 + eps1 * cosAMax;
            float sinA = sqrtf(1.0f - cosA*cosA);
            float phi = 2 * kPI * eps2;
            vec3 l = vec3_add(vec3_mult(su, cosf(phi) * sinA), vec3_add(vec3_mult(sv, sin(phi) * sinA), vec3_mult(sw, cosA)));
            l = vec3_norml(l);
            
            // shoot shadow ray
            Hit lightHit;
            int hitID;
            Ray r = ray_new(rec->pos, l);
            (*inoutRayCount)++;
            if (HitWorld(&r, kMinT, kMaxT, &lightHit, &hitID) && hitID == i)
            {
                float omega = 2 * kPI * (1-cosAMax);
                
                vec3 rdir = r_in->dir;
                AssertUnit(rdir);
                vec3 nl = vec3_dot(rec->normal, rdir) < 0 ? rec->normal : vec3_neg(rec->normal);
                *outLightE = vec3_add(*outLightE, vec3_mult(vec3_prod(mat->albedo, smat->emissive), maxf(0.0f, vec3_dot(l, nl)) * omega / kPI));
            }
        }
#endif
        return true;
    }
    else if (mat->type ==Metal)
    {
        AssertUnit(r_in->dir); AssertUnit(rec->normal);
        vec3 refl = vec3_reflect(r_in->dir, rec->normal);
        // reflected ray, and random inside of sphere based on roughness
        *scattered = ray_new(rec->pos, vec3_norml(vec3_add(refl, vec3_mult(RandomInUnitSphere(), mat->roughness))));
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
        if (RandomFloat01() < reflProb)
            *scattered = ray_new(rec->pos, vec3_norml(refl));
        else
            *scattered = ray_new(rec->pos, vec3_norml(refr));
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

//static enkiTaskScheduler* g_TS;

//void InitializeTest()
//{
//    g_TS = enkiNewTaskScheduler();
//    enkiInitTaskScheduler(g_TS);
//}

//void ShutdownTest()
//{
//    enkiDeleteTaskScheduler(g_TS);
//}

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
                float u = ((float)x + RandomFloat01()) * invWidth;
                float v = ((float)y + RandomFloat01()) * invHeight;
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
    vec3 lookfrom = {0.0, 2.0, 3.0};
    vec3 lookat = {0.0, 0.0, 0.0};
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
}

int main(int argc, char** argv) 
{   
    int ray_count = 0;
    uint32_t width = 400, height = 400;
    if (argc > 1) width = (uint32_t)atoi(argv[1]);
    if (argc > 2) height = (uint32_t)atoi(argv[2]);
    float* backbuffer = (float*)malloc(sizeof(float) * width * height * 4);
    printf("Rendering path traced scene...\nwidth: %d\nheight: %d\n", 
            width, height);
    DrawTest(0.0, 1, width, height, backbuffer, &ray_count);

    bmp_t bmp = bmp_new(width, height, 4);
    for (int i = 0; i < width * height * 4; i++) {
        bmp.pixels[i] = (uint8_t)(backbuffer[i] * 255.0f);
    }
    bmp_t tmp = bmp_flip_vertical(&bmp);
    bmp_write("out.png", &tmp);
    bmp_free(&tmp);
    bmp_free(&bmp);
    free(backbuffer);
    return 0;
}
