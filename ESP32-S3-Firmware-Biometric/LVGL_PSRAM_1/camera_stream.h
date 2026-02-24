#ifndef CAMERA_STREAM_H
#define CAMERA_STREAM_H

#include <lvgl.h>
#include "esp_camera.h"



// Định nghĩa chân camera theo cấu hình code cũ
#define PWDN_GPIO   -1
#define RESET_GPIO  -1
#define XCLK_GPIO   15
#define SIOD_GPIO   4
#define SIOC_GPIO   5
#define Y9_GPIO     16
#define Y8_GPIO     17
#define Y7_GPIO     18
#define Y6_GPIO     12
#define Y5_GPIO     10
#define Y4_GPIO     8
#define Y3_GPIO     9
#define Y2_GPIO     11
#define VSYNC_GPIO  6
#define HREF_GPIO   7
#define PCLK_GPIO   13


extern lv_obj_t* camera_img_obj;

// Khởi tạo camera với cấu hình được xác định
void init_camera();

// Tạo widget (lv_img) để hiển thị stream từ camera
lv_obj_t* create_camera_widget(lv_obj_t* parent);

// Cập nhật widget camera với khung hình mới (sử dụng JPEG)
void update_camera_widget(lv_obj_t* img);
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);

void update_ui_task(void *param);

#endif
