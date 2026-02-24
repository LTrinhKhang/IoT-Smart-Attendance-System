#include "display_touch.h"
#include <SPI.h>
#include "lvgl_setup.h"


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

void init_display() {
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(1);
    tft.invertDisplay(true);
    tft.setSPISpeed(40 * 1000000);
    Serial.println("Màn hình ILI9341 đã khởi động!");
}

void init_touch() {
    ts.begin();
    ts.setRotation(3);
    Serial.println("Cảm ứng XPT2046 đã khởi động!");
}

void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((uint16_t *)color_p, w * h);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        data->state = LV_INDEV_STATE_PR;
        data->point.x = map(p.x, 200, 3800, 0, SCREEN_WIDTH);
        data->point.y = map(p.y, 200, 3800, 0, SCREEN_HEIGHT);
        // Serial.printf("Chạm tại: (%d, %d)\n", data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
