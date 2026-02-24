#ifndef LVGL_SETUP_H
#define LVGL_SETUP_H

#include <lvgl.h>

#define BUFFER_SIZE ((size_t)(2 * 1024 * 1024) / sizeof(lv_color_t))

// Kích thước màn hình
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

extern lv_color_t *buf1;
extern lv_color_t *buf2;
extern lv_disp_draw_buf_t draw_buf;
extern lv_disp_drv_t disp_drv;

void init_lvgl_buffers();
void init_lvgl();

#endif
