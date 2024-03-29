#include <tracy.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define CLMPF(x) ((x) * ((x) < 1.0) * ((x) > 0.0) + (float)((x) >= 1.0))

typedef struct JobInfo {
    Render3D* render;
    Scene3D* scene;
    uint32_t start;
    uint32_t end;
} JobInfo;

static JobInfo render3D_job_info(const Render3D* restrict render, const Scene3D* restrict scene, const uint32_t start, const uint32_t end)
{
    JobInfo job;
    job.render = (Render3D*)(size_t)render;
    job.scene = (Scene3D*)(size_t)scene;
    job.start = start;
    job.end = end;
    return job;
}

__attribute__((__unused__))
static inline vec3 vec3_tonemap(const vec3 x)
{
    vec3 A = vec3_prod(x, vec3_add(vec3_mult(x, 2.51f), vec3_uni(0.03f)));
    vec3 B = vec3_add(vec3_prod(x, vec3_add(vec3_mult(x, 2.43f), vec3_uni(0.59f))), vec3_uni(0.14f));
    return _vec3_op(A, /, B);
}

static void* render3D_render_job(void* arg)
{
    const JobInfo job = *(JobInfo*)arg;
    const uint32_t width = job.render->width;
    const uint32_t height = job.render->height;
    const uint32_t spp = job.render->spp;
    
#ifdef TRACY_PERF

    static volatile bool first = true;
    static volatile uint32_t frame = 1;
    
    bool check = first;
    first = false;

    double time = 0.0;
    if (check) {
        time = time_clock();
    }

#endif

    uint8_t* backbuffer = job.render->buffer + job.start * width * 4;
    const float invSpp = 1.0f / (float)spp;
    const float invWidth = 1.0f / width;
    const float invHeight = 1.0f / height;
    const float colFac = 1.0 / (float)(job.render->timer + 1);
    const float prevFac = 1.0 - colFac;

    for (uint32_t y = job.start; y < job.end; ++y) {
        for (uint32_t x = 0; x < job.render->width; ++x) {
            vec3 col = {0.0, 0.0, 0.0};
            for (uint32_t s = 0; s < spp; s++) {
                float u = ((float)x + frand_norm()) * invWidth;
                float v = ((float)y + frand_norm()) * invHeight;
                Ray3D r = cam3D_ray(&job.scene->cam, u, v);
                col = vec3_add(col, ray3D_trace(job.scene, &r, 0));
            }

            col = (vec3){sqrtf(col.x * invSpp), sqrtf(col.y * invSpp), sqrtf(col.z * invSpp)};
            
            vec3 prev = {(float)backbuffer[0] / 255.0, (float)backbuffer[1] / 255.0, (float)backbuffer[2] / 255.0};
            col = vec3_add(_vec3_mult(prev, prevFac), _vec3_mult(col, colFac));
            //col = vec3_tonemap(col);

            backbuffer[0] = (unsigned)(CLMPF(col.x) * 255.0);
            backbuffer[1] = (unsigned)(CLMPF(col.y) * 255.0);
            backbuffer[2] = (unsigned)(CLMPF(col.z) * 255.0);
            backbuffer[3] = 255;
            backbuffer += 4;

#ifdef TRACY_PERF
            
            if (check) {
                double time_elapsed = time_clock() - time;
                uint32_t samp = (y - job.start) * width + x + 1, off = (job.end - job.start) * width;
                float perc = ((float)samp / (float)off) * 100.0f;
                double time_estimate = time_elapsed * 100.0f / perc;
                double time_remaining = time_estimate - time_elapsed;
                printf("\rframe\t%d\t%.01f%%\t( %u\t/ %u\t)\t%.01fs\t\t%.01fs\t\t%.01fs", frame, perc, samp, off, time_elapsed, time_estimate, time_remaining);
            }

#endif

        }
    }

#ifdef TRACY_PERF

    if (check) {
        frame++;
        first = true;
        printf("\n");
    }

#endif

    return NULL;
}

void render3D_render(const Render3D* restrict render, const Scene3D* restrict scene)
{
    const uint32_t thread_count = render->threads;
    const uint32_t chunk = render->height / thread_count;
    
    uint32_t start = 0, end = chunk;

    pthread_t threads[thread_count - 1];
    JobInfo jobs[thread_count];

    for (uint32_t i = 0; i < thread_count - 1; i++) {
        jobs[i] = render3D_job_info(render, scene, start, end);
        pthread_create(&threads[i], NULL, &render3D_render_job, &jobs[i]);
        start += chunk;
        end += chunk;
    }
    
    jobs[thread_count - 1] = render3D_job_info(render, scene, start, render->height);
    render3D_render_job(&jobs[thread_count - 1]);

    for (uint32_t i = 0; i < thread_count - 1; i++) {
        pthread_join(threads[i], NULL);
    }
}

Render3D render3D_new(const uint32_t width, const uint32_t height, const uint32_t spp)
{
    Render3D render;
    render.buffer = NULL;
    render.width = width;
    render.height = height;
    render.spp = spp;
    render.frames = 1;
    render.threads = 1;
    render.timer = 0;
    return render;
}

void render3D_set(Render3D* render)
{
    render->buffer = calloc(render->width * render->height * 4, sizeof(uint8_t));
}

void render3D_free(Render3D* render)
{
    if (render && render->buffer) {
        free(render->buffer);
    }
}

bmp_t render3D_bmp(const Render3D* restrict render, const Scene3D* restrict scene)
{   
    render3D_render(render, scene);
    
    bmp_t tmp;
    tmp.channels = 4;
    tmp.width = render->width;
    tmp.height = render->height;
    tmp.pixels = render->buffer;
    
    return bmp_flip_vertical(&tmp);
}
