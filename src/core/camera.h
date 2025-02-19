#ifndef CAMERA_H_
#define CAMERA_H_

#include "vec.h"

#include <stdint.h>

typedef struct {
    Mat4 view;
    Mat4 proj;

    Mat4 invView;
    Mat4 invProj;
} CameraData;

void camera_resize(uint32_t width, uint32_t height);

void camera_update(float dt);

CameraData* camera_get_data();

#endif // CAMERA_H_
