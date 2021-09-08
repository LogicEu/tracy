#include <tracy.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define CLMPF(x) ((x) * ((x) < 1.0) + (float)(x >= 1.0))

JobData job;

static void* frame_render_job(void* args)
{
    uint32_t* uarg = args;
    uint32_t start = *(uarg++);
    uint32_t end = *uarg;
    
#if PERF
    static volatile bool first = true;
    static volatile int frame = 1;
    bool check = first;
    first = false;

    double time;
    if (check) time = time_clock();
    float lerpFac = (float)frame / (float)(frame + 1);
#else  
    float lerpFac = (float)job.frameCount / (float)(job.frameCount + 1);
#endif
    float* backbuffer = job.backbuffer + start * job.screenWidth * 3;
    float invWidth = 1.0f / job.screenWidth;
    float invHeight = 1.0f / job.screenHeight;
    lerpFac *= animate_smoothing;

    int rayCount = 0;
    for (uint32_t y = start; y < end; ++y) {
        for (int x = 0; x < job.screenWidth; ++x) {
            vec3 col = {0.0, 0.0, 0.0};
            for (int s = 0; s < samples_per_pixel; s++) {
                float u = ((float)x + randf_norm()) * invWidth;
                float v = ((float)y + randf_norm()) * invHeight;
                Ray3D r = camera_ray(job.cam, u, v);
                col = vec3_add(col, ray_trace(&r, 0, &rayCount));
            }
            col = vec3_mult(col, 1.0f / (float)samples_per_pixel);
            //col = vec3_mult(col, 1.0f / (float)frame);
            col = vec3_new(sqrtf(col.x), sqrtf(col.y), sqrtf(col.z));
            
            vec3 prev = vec3_new(backbuffer[0], backbuffer[1], backbuffer[2]);
            col = vec3_add(vec3_mult(prev, lerpFac), vec3_mult(col, (1.0f - lerpFac)));
            
            //if (col.x > backbuffer[0]) backbuffer[0] = minf(backbuffer[0] + col.x / frame, 1.0);
            //if (col.y > backbuffer[1]) backbuffer[1] = minf(backbuffer[1] + col.y / frame, 1.0);
            //if (col.z > backbuffer[2]) backbuffer[2] = minf(backbuffer[2] + col.z / frame, 1.0);
            
            backbuffer[0] = CLMPF(col.x);
            backbuffer[1] = CLMPF(col.y);
            backbuffer[2] = CLMPF(col.z);
            backbuffer += 3;
#if PERF
            if (check) {
                double time_elapsed = time_clock() - time;
                int samp = (y - start) * job.screenWidth + x + 1, off = (end - start) * job.screenWidth;
                float perc = ((float)samp / (float)off) * 100.0f;
                double time_estimate = time_elapsed * 100.0f / perc;
                double time_remaining = time_estimate - time_elapsed;
                printf("\rframe\t%d\t%.01f%%\t( %d\t/ %d\t)\t%.01fs\t\t%.01fs\t\t%.01fs", frame, perc, samp, off, time_elapsed, time_estimate, time_remaining);
            }
#endif
        }
    }
#if PERF
    if (check) {
        frame++;
        first = true;
        printf("\n");
    }
#endif
    job.rayCount += rayCount;
    return NULL;
}

void frame_render(int thread_count)
{
    uint32_t chunk = job.screenHeight / thread_count;
    uint32_t start = 0, end = chunk;

    pthread_t threads[thread_count - 1];
    uint32_t args[thread_count][2];
    for (int i = 0; i < thread_count - 1; i++) {
        args[i][0] = start;
        args[i][1] = end;
        pthread_create(&threads[i], NULL, *frame_render_job, &args[i][0]);
        start += chunk;
        end += chunk;
    }

    args[thread_count - 1][0] = start;
    args[thread_count - 1][1] = end;
    frame_render_job(&args[thread_count - 1][0]);

    for (int i = 0; i < thread_count - 1; i++) {
        pthread_join(threads[i], NULL);
    }
}
