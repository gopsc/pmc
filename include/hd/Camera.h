#pragma once

/* Camera */
#define cimg_display 0 /* 不显示图像，只保存 */
#include "CImg.h"

void Camera_init(int w, int h);
void Camera_close();
bool Camera_is_open();

/* 在线程安全的情况下取得一帧图像 */
cimg_library::CImg<unsigned char> get_frame_from_camera(bool& flag);