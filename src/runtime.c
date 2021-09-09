#include <tracy.h>
#include <imgtool.h>
#include <gleex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int vao, vbo, shader;
unsigned int vao3D, vbo3D, shader3D;

void textureRebind(texture_t tex, bmp_t* bmp)
{
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp->width, bmp->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void vertexBuffersInit(unsigned int* VAO, unsigned int* VBO, unsigned int attribute, unsigned int stride)
{
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * stride, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(attribute);
    glVertexAttribPointer(attribute, stride, GL_FLOAT, GL_FALSE, stride * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void vertexBuffersSpheres(unsigned int* VAO, unsigned int* VBO, unsigned int attribute)
{
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, spheres->used * spheres->bytes, spheres->data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(attribute);
    glVertexAttribPointer(attribute, spheres->bytes / sizeof(float), GL_FLOAT, GL_FALSE, spheres->bytes, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void textureDraw(texture_t tex, vec2 pos, float rot)
{
    float w = (float)tex.width * 0.5f;
    float h = (float)tex.height * 0.5f;

    vec2 p1 = vec2_add(pos, vec2_new(-w, -h));
    vec2 p2 = vec2_add(pos, vec2_new(-w, h));
    vec2 p3 = vec2_add(pos, vec2_new(w, h));
    vec2 p4 = vec2_add(pos, vec2_new(w, -h));

    //CHANGE
    float vertices[] = {
        p1.x, p1.y, 0.0f, 0.0f,
        p2.x, p2.y, 0.0f, 1.0f,
        p3.x, p3.y, 1.0f, 1.0f,
        p1.x, p1.y, 0.0f, 0.0f,
        p3.x, p3.y, 1.0f, 1.0f,
        p4.x, p4.y, 1.0f, 0.0f
    };

    glBindTexture(GL_TEXTURE_2D, tex.id);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

void frameDraw(float w, float h)
{
    float vertices[] = {
        0.0, h, 0.0f, 0.0f,
        0.0, 0.0, 0.0f, 1.0f,
        w, 0.0, 1.0f, 1.0f,
        0.0, h, 0.0f, 0.0f,
        w, 0.0, 1.0f, 1.0f,
        w, h, 1.0f, 0.0f
    };

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void bmp_map(bmp_t* bmp, float* backbuffer)
{
    for (uint32_t i = 0; i < bmp->width * bmp->height; i++) {
        bmp->pixels[i * 4 + 0] = (uint8_t)(backbuffer[i * 3 + 0] * 255.0f);
        bmp->pixels[i * 4 + 1] = (uint8_t)(backbuffer[i * 3 + 1] * 255.0f);
        bmp->pixels[i * 4 + 2] = (uint8_t)(backbuffer[i * 3 + 2] * 255.0f);
        bmp->pixels[i * 4 + 3] = 255;
    }
}

static void bmp_blend(bmp_t* bmp, float* backbuffer, int frame)
{
    for (uint32_t i = 0; i < bmp->width * bmp->height; i++) {
        bmp->pixels[i * 4 + 0] = (uint8_t)clampf(lerpf(backbuffer[i * 3 + 0] * 255.0f, (float)bmp->pixels[i * 4 + 0], 1.0 - 1.0 / (float)frame), (float)bmp->pixels[i * 4 + 0], 255.0);
        bmp->pixels[i * 4 + 1] = (uint8_t)clampf(lerpf(backbuffer[i * 3 + 1] * 255.0f, (float)bmp->pixels[i * 4 + 1], 1.0 - 1.0 / (float)frame), (float)bmp->pixels[i * 4 + 1], 255.0);
        bmp->pixels[i * 4 + 2] = (uint8_t)clampf(lerpf(backbuffer[i * 3 + 2] * 255.0f, (float)bmp->pixels[i * 4 + 2], 1.0 - 1.0 / (float)frame), (float)bmp->pixels[i * 4 + 2], 255.0);
        //bmp->pixels[i * 4 + 3] = 255;
    }
}

static void bmp_blend_(bmp_t* bmp, float* bigbuffer, int iters)
{
    for (uint32_t i = 0; i < bmp->width * bmp->height; i++) {
        //bmp->pixels[i * 4 + 0] = (uint8_t)_clampf(bigbuffer[i * 3 + 0] * 255.0, bmp->pixels[i * 4 + 0], 255.0);
        //bmp->pixels[i * 4 + 1] = (uint8_t)_clampf(bigbuffer[i * 3 + 1] * 255.0, bmp->pixels[i * 4 + 1], 255.0);
        //bmp->pixels[i * 4 + 2] = (uint8_t)_clampf(bigbuffer[i * 3 + 2] * 255.0, bmp->pixels[i * 4 + 2], 255.0);
        
        bmp->pixels[i * 4 + 0] = (uint8_t)(bigbuffer[i * 3 + 0] * 255.0);
        bmp->pixels[i * 4 + 1] = (uint8_t)(bigbuffer[i * 3 + 1] * 255.0);
        bmp->pixels[i * 4 + 2] = (uint8_t)(bigbuffer[i * 3 + 2] * 255.0);
    }
}

static void frame_add(float* backbuffer, float* bigbuffer, int width, int height, int iters)
{
    float lerpFac = (float)iters / (float)(iters + 1);
    for (int i = 0; i < width * height * 3; i++) {
        bigbuffer[i] = _clampf(backbuffer[i] * lerpFac + bigbuffer[i] * (1.0f - lerpFac), bigbuffer[i], 1.0);
    }
}

int main(int argc, char** argv)
{
    int width = 800, height = 600, fullscreen = 0;
    if (argc > 1) width = atoi(argv[1]);
    if (argc > 2) height = atoi(argv[2]);
    if (argc > 3) fullscreen++;
    samples_per_pixel = 1;

    float res[] = {width, height};
    mat4 projection = mat4_ortho(0.0f, res[0], 0.0f, res[1]);

    glee_init();
    glee_window_create("tracy path tracer", width, height, fullscreen, 0);
    shader = glee_shader_load("shaders/vertex.glsl", "shaders/fragment.glsl");
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, &projection.data[0][0]);
    vertexBuffersInit(&vao, &vbo, 0, 4);

    scene_init();
    shader3D = glee_shader_load("shaders/tracyv.glsl", "shaders/tracyf.glsl");
    glee_shader_uniform_set(shader3D, 2, "u_resolution", &res[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader3D, "projection"), 1, GL_FALSE, &projection.data[0][0]);
    //vertexBuffersSpheres(&vao3D, &vbo3D, 1);

    // tracy environment //

    cam = camera_new(lookfrom, lookat, vec3_new(0.0, 1.0, 0.0), fov, (float)width / (float)height, aperture, distToFocus);
    float* bigbuffer = (float*)calloc(width * height * 3, sizeof(float));
    float* backbuffer = (float*)calloc(width * height * 3, sizeof(float));
    job.frameCount = 1;
    job.screenWidth = width;
    job.screenHeight = height;
    job.backbuffer = backbuffer;
    job.cam = &cam;
    job.rayCount = 0;

    int iters = 2;
    frame_render(16);
    frame_add(backbuffer, bigbuffer, width, height, iters);

    bmp_t bmp = bmp_new(width, height, 4);
    bmp_map(&bmp, backbuffer);

    texture_t tex = texture_from_bmp(&bmp);
    //texture_t tex = texture_load("assets/textures/sprite.png");
    vec2 pos = {(float)width * 0.5, (float)height * 0.5}, mouse, mousePos;
    glee_mouse_pos_3d(&mouse.x, &mouse.y);
    float Dtime = glee_time_get();

    glee_screen_color(0.3, 0.3, 1.0, 1.0);
    while (glee_window_is_open()) {
        glee_screen_clear();
        float delta_time = glee_time_delta(&Dtime);
        float time = glee_time_get();
        float delta_speed = delta_time * 4;

        glee_mouse_pos_3d(&mousePos.x, &mousePos.y);
        vec2 mouseCursor = vec2_sub(mouse, mousePos);

        if (glee_key_pressed(GLFW_KEY_ESCAPE)) break;
        if (glee_key_down(GLFW_KEY_W)) {
            lookfrom.z += delta_speed;
            iters = 1;
        }
        if (glee_key_down(GLFW_KEY_S)) {
            lookfrom.z -= delta_speed;
            iters = 1;
        }
        if (glee_key_down(GLFW_KEY_D)) {
            lookfrom.x += delta_speed;
            iters = 1;
        }
        if (glee_key_down(GLFW_KEY_A)) {
            lookfrom.x -= delta_speed;
            iters = 1;
        }
        if (glee_key_down(GLFW_KEY_Z)) {
            lookfrom.y -= delta_speed;
            iters = 1;
        }
        if (glee_key_down(GLFW_KEY_X)) {
            lookfrom.y += delta_speed;
            iters = 1;
        }
        if (iters == 1) {
            memset(backbuffer, 0, width * height * 3 * sizeof(float));
            memset(bigbuffer, 0, width * height * 3 * sizeof(float));
        }
        
        //vec3_print(lookfrom);
        lookat.x = mouseCursor.x;
        lookat.y = mouseCursor.y;
        cam = camera_new(lookfrom, lookat, vec3_new(0.0, 1.0, 0.0), fov, (float)width / (float)height, aperture, distToFocus);
        //glUseProgram(shader);
        //textureDraw(tex, pos, 0.0);

        glUseProgram(shader3D);
        glee_shader_uniform_set(shader3D, 3, "u_position", &lookfrom);
        glee_shader_uniform_set(shader3D, 3, "u_lookat", &lookat);
        glee_shader_uniform_set(shader3D, 1, "u_time", &time);
        frameDraw(width, height);

        //frame_render(16);
        //frame_add(backbuffer, bigbuffer, width, height, iters);

        //bmp_blend_(&bmp, bigbuffer, iters);
        //textureRebind(tex, &bmp);

        iters++;
        glee_screen_refresh();
    }
    bmp_write("out.png", &bmp);
    bmp_free(&bmp);
    free(backbuffer);
    glee_deinit();
    return EXIT_SUCCESS;
}
