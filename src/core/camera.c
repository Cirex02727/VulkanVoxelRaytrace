#include "camera.h"

#include "input.h"

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <math.h>

static Vec3 globalUp = { 0.0f, 1.0f, 0.0f };

static Vec3 cameraPosition = { 1.463829f, 1.266712f, 0.486529f }, cameraDirection = { 0.0f, 0.0f, 0.0f };
static float cameraYaw = 56.999176f, cameraPitch = 17.666922f;

static Vec3 cameraFront, cameraRight;
static Mat4 cameraProjView, cameraUiProj;

static CameraData cameraData;

static bool cameraMoved = false;

static void camera_update_vectors()
{
    Vec3 front;
    front.x = cos(deg_to_rad(cameraYaw)) * cos(deg_to_rad(cameraPitch));
    front.y = sin(deg_to_rad(cameraPitch));
    front.z = sin(deg_to_rad(cameraYaw)) * cos(deg_to_rad(cameraPitch));
    vec3_normalize(&front, &cameraDirection);
    
    vec3_cross(&cameraDirection, &globalUp, &cameraRight);
    vec3_normalize(&cameraRight, &cameraRight);
}

static void camera_calculate_view()
{
    vec3_add(&cameraPosition, &cameraDirection, &cameraFront);
    mat4_lookat(&cameraPosition, &cameraFront, &globalUp, &cameraData.view);
    mat4_inv(&cameraData.view, &cameraData.invView);
}

static void camera_calculate_proj_view()
{
    mat4_mul(&cameraData.proj, &cameraData.view, &cameraProjView);
}

void camera_resize(uint32_t width, uint32_t height)
{
    mat4_perspective(45.0f, (float)width / (float)height, 0.01f, 10000.0f, &cameraData.proj);
    mat4_ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f, &cameraUiProj);

    mat4_inv(&cameraData.proj, &cameraData.invProj);

    camera_update_vectors();

    camera_calculate_view();
    camera_calculate_proj_view();
}

void camera_update(float dt)
{
    // printf("%ff, %ff, %ff, | %ff | %ff\n", cameraPosition.x, cameraPosition.y, cameraPosition.z, cameraYaw, cameraPitch);

    float speed = 2.0f * dt, sensitivity = 20.0f * dt;
    if(input_key(GLFW_KEY_LEFT_CONTROL))
        speed *= 3.0f;
    
    // Update mouse
    Vec2 mouse_movement = input_mouse_movement();
    mouse_movement.x *= sensitivity;
    mouse_movement.y *= sensitivity;

    cameraYaw   += mouse_movement.x;
    cameraPitch += mouse_movement.y;
    
    cameraYaw   = fmodf(cameraYaw, 360.0f);
    cameraPitch = clamp(cameraPitch, -89.0f, 89.0f);

    // Reload orientation
    camera_update_vectors();

    Vec3 scaledFront, scaledRight, scaledUp;

    scaledFront = (Vec3) { cameraDirection.x, 0.0f, cameraDirection.z };
    vec3_normalize(&scaledFront, &scaledFront);

    vec3_scale(&scaledFront, speed, &scaledFront);
    vec3_scale(&cameraRight, speed, &scaledRight);
    vec3_scale(&globalUp,    speed, &scaledUp);

    // Update keyboard
    Vec3 movement = {0};
    if(input_key(GLFW_KEY_W))
        vec3_add(&movement, &scaledFront, &movement);
    if(input_key(GLFW_KEY_S))
        vec3_sub(&movement, &scaledFront, &movement);
    if(input_key(GLFW_KEY_A))
        vec3_sub(&movement, &scaledRight, &movement);
    if(input_key(GLFW_KEY_D))
        vec3_add(&movement, &scaledRight, &movement);
    
    if(input_key(GLFW_KEY_SPACE))
        vec3_sub(&movement, &scaledUp, &movement);
    if(input_key(GLFW_KEY_LEFT_SHIFT))
        vec3_add(&movement, &scaledUp, &movement);
    
    cameraMoved = !vec3_is_zero(&movement) || !vec2_is_zero(&mouse_movement);
    if(cameraMoved)
    {
        vec3_add(&cameraPosition, &movement, &cameraPosition);
        camera_calculate_view();
        camera_calculate_proj_view();
    }
}

CameraData* camera_get_data()
{
    return &cameraData;
}

bool camera_moved()
{
    return cameraMoved;
}
