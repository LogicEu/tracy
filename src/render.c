#include <tracy.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define CLMPF(x) ((x) * ((x) < 1.0) + (float)((x) >= 1.0))

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

    double time;
    if (check) {
        time = time_clock();
    }

#endif

    static const float animate_smoothing = 0.01f;

    float* backbuffer = job.render->buffer + job.start * width * 3;
    const float invWidth = 1.0f / width;
    const float invHeight = 1.0f / height;
    float lerpFac = (float)job.render->frames / (float)(job.render->frames + 1);
    
    lerpFac *= animate_smoothing;

    for (uint32_t y = job.start; y < job.end; ++y) {
        for (uint32_t x = 0; x < job.render->width; ++x) {
            vec3 col = {0.0, 0.0, 0.0};
            for (uint32_t s = 0; s < spp; s++) {
                float u = ((float)x + randf_norm()) * invWidth;
                float v = ((float)y + randf_norm()) * invHeight;
                Ray3D r = cam3D_ray(&job.scene->cam, u, v);
                col = vec3_add(col, ray3D_trace(job.scene, &r, 0));
            }
            col = vec3_mult(col, 1.0f / (float)spp);
            col = vec3_new(sqrtf(col.x), sqrtf(col.y), sqrtf(col.z));
            
            vec3 prev = vec3_new(backbuffer[0], backbuffer[1], backbuffer[2]);
            col = vec3_add(vec3_mult(prev, lerpFac), vec3_mult(col, (1.0f - lerpFac)));
            backbuffer[0] = CLMPF(col.x);
            backbuffer[1] = CLMPF(col.y);
            backbuffer[2] = CLMPF(col.z);
            backbuffer += 3;

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

static void render3D_render_dispatch(const Render3D* restrict render, const Scene3D* restrict scene)
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
    return render;
}

void render3D_set(Render3D* render)
{
    render->buffer = calloc(render->width * render->height * 3, sizeof(float));
}

void render3D_free(Render3D* render)
{
    if (!render) return;
    free(render->buffer);
}

bmp_t render3D_render(const Render3D* restrict render, const Scene3D* restrict scene)
{
    const size_t buff_len = render->width * render->height * 3;
    
    memset(render->buffer, 0, buff_len * sizeof(float));
    render3D_render_dispatch(render, scene);

    bmp_t tmp = bmp_new(render->width, render->height, 3);
    for (uint32_t j = 0; j < buff_len; j++) {
        tmp.pixels[j] = (uint8_t)(render->buffer[j] * 255.0f);
    }
    bmp_t bmp = bmp_flip_vertical(&tmp);
    bmp_free(&tmp);
    
    return bmp;
}
