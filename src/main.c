#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "core/window.h"
#include "core/camera.h"
#include "core/input.h"
#include "core/core.h"
#include "core/log.h"

#include "render/vulkan_globals.h"
#include "render/vulkan.h"

#include <stdio.h>

#define TITLE "Vulkan"

int main()
{
    // GLFW/GLEW init
    {
        int error = glfwInit();
        if (error != GLFW_TRUE)
        {
            log_error("GLFW failed to init %d", error);
            return 1;
        }
    }
    
    if(!window_create(TITLE, 1000, 600))
        return 1;
    
    if (glfwVulkanSupported() != GLFW_TRUE)
    {
        log_error("GLFW vulkan is not supported!");
        return 1;
    }

    window_init();

    if(!vulkan_init(TITLE))
        return 1;
    
    char tempString[1024];

    float prevTime = 0.0, minDelta = 1.0 / 60.0f;

    while (!window_should_close())
    {
        float deltaTime = glfwGetTime() - prevTime;
        if(deltaTime < minDelta)
        {
            glfwPollEvents();
            continue;
        }

        prevTime = glfwGetTime();
        
        // Update
        {
            camera_update(deltaTime);

            sprintf(tempString, "Vulkan %.1f (%.2fms)", 1.0f / deltaTime, deltaTime);
            window_set_title(tempString);
        }

        // Render
        // TODO: Clear buffers
        
        {
            if(!vulkan_draw_frame())
                return 1;
        }
        
        input_update();
        glfwPollEvents();

        window_swap_buffers();
    }

    vulkan_destroy();

    window_destroy();
    
    glfwTerminate();
    return 0;
}
