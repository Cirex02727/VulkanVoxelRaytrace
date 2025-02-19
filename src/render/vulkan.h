#ifndef VULKAN_BACKEND_H_
#define VULKAN_BACKEND_H_

#include <stdbool.h>

bool vulkan_init(const char* title);

void vulkan_destroy();

void vulkan_resize();

bool vulkan_draw_frame();

#endif // VULKAN_BACKEND_H_
