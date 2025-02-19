#ifndef WINDOW_H_
#define WINDOW_H_

#include "vec.h"

#include <GLFW/glfw3.h>

#include <stdbool.h>

typedef struct {
    UIVec2 size;
} WindowStats;

bool window_create(const char* title, uint32_t width, uint32_t height);

void window_init();

bool window_should_close();

void window_swap_buffers();

void window_destroy();

GLFWwindow* window_get_handle();

void window_get_window_size(UIVec2* size);

void window_set_title(const char* title);

#endif // WINDOW_H_
