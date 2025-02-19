#include "window.h"

#include "camera.h"
#include "render/vulkan.h"
#include "input.h"

#include <stdlib.h>
#include <stdio.h>

static GLFWwindow* window;

static WindowStats* windowStats;

static void window_resize(GLFWwindow* window, int width, int height)
{
    WindowStats* stats = (WindowStats*) glfwGetWindowUserPointer(window);
    stats->size = (UIVec2) { width, height };

    // TODO: Viewport Resize
    camera_resize(width, height);

    vulkan_resize();
}

bool window_create(const char* title, uint32_t width, uint32_t height)
{
    glfwWindowHint(GLFW_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);   

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if(window == NULL)
    {
        printf("[ERROR] GLFW create window!\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    windowStats = (WindowStats*) malloc(sizeof(WindowStats));
    windowStats->size = (UIVec2) { width, height };

    glfwSetWindowUserPointer(window, windowStats);
    return true;
}

void window_init()
{
    glfwSetWindowSizeCallback(window, window_resize);

    int vSync = 0;
    glfwSwapInterval(vSync);

    input_init(window);
    
    camera_resize(windowStats->size.x, windowStats->size.y);
}

bool window_should_close()
{
    return glfwWindowShouldClose(window);
}

void window_swap_buffers()
{
    glfwSwapBuffers(window);
}

void window_destroy()
{
    glfwDestroyWindow(window);
}

GLFWwindow* window_get_handle()
{
    return window;
}

void window_get_window_size(UIVec2* size)
{
    *size = windowStats->size;
}

void window_set_title(const char* title)
{
    glfwSetWindowTitle(window, title);
}
