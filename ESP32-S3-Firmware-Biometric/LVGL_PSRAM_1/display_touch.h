#ifndef DISPLAY_TOUCH_H
#define DISPLAY_TOUCH_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>

// 🔹 Khai báo chân kết nối LCD ILI9341
#define TFT_CS   38  
#define TFT_DC   39  
#define TFT_RST  40  
#define TFT_SCK  41  
#define TFT_MISO 42  
#define TFT_MOSI 21  

// 🔹 Khai báo chân cảm ứng XPT2046
#define TOUCH_CS  20   
#define TOUCH_IRQ  -1  

extern Adafruit_ILI9341 tft;
extern XPT2046_Touchscreen ts;

void init_display();
void init_touch();
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

#endif
