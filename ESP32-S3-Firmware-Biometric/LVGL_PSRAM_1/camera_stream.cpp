#include "camera_stream.h"
#include <Arduino.h>
#include "esp_camera.h"
#include <TJpg_Decoder.h>  // Đảm bảo đã include thư viện TJpg_Decoder
#include "display_touch.h"
#include "lvgl.h"






// Lưu toạ độ của widget (góc trên bên trái)
static int widget_offset_x;
static int widget_offset_y;
static int crop_offset_x = 60;  // (320-200)/2
static int crop_offset_y = 20;  // (240-200)/2

lv_obj_t* camera_img_obj;

void init_camera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO;
    config.pin_d1 = Y3_GPIO;
    config.pin_d2 = Y4_GPIO;
    config.pin_d3 = Y5_GPIO;
    config.pin_d4 = Y6_GPIO;
    config.pin_d5 = Y7_GPIO;
    config.pin_d6 = Y8_GPIO;
    config.pin_d7 = Y9_GPIO;
    config.pin_xclk = XCLK_GPIO;
    config.pin_pclk = PCLK_GPIO;
    config.pin_vsync = VSYNC_GPIO;
    config.pin_href = HREF_GPIO;
    config.pin_sscb_sda = SIOD_GPIO;
    config.pin_sscb_scl = SIOC_GPIO;
    config.pin_pwdn = PWDN_GPIO;
    config.pin_reset = RESET_GPIO;
    config.pixel_format = PIXFORMAT_JPEG;
    // Nếu board không có PSRAM, chuyển sang sử dụng DRAM
    config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
    config.frame_size = FRAMESIZE_QVGA;
    config.fb_count = 2;
    config.jpeg_quality =12;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.xclk_freq_hz = 20000000;
    
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
    } else {
        Serial.println("Camera initialized!");
        sensor_t * s = esp_camera_sensor_get();
        if (s) {
            // Tăng độ sáng và độ tương phản
            s->set_brightness(s, 1);  // Tăng độ sáng (từ -2 đến 2)
            s->set_contrast(s, 1);    // Tăng độ tương phản (từ -2 đến 2)
            
            // Giảm nhiễu
            s->set_denoise(s, 1);
            
            // Cân bằng trắng tự động
            s->set_whitebal(s, 1);
            s->set_awb_gain(s, 1);
            
            // Tối ưu cho nhận diện khuôn mặt
            s->set_saturation(s, 0);  // Giảm độ bão hòa màu
            s->set_sharpness(s, 1);   // Tăng độ sắc nét
            
            Serial.println("Camera settings adjusted for face recognition");
        }
    }
}


lv_obj_t* create_camera_widget(lv_obj_t* parent) {
    lv_obj_t* img = lv_img_create(parent);
    lv_obj_set_size(img, 200, 200);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_area_t coords;
    lv_obj_get_coords(img, &coords);

    // Tính toán widget_offset_x và widget_offset_y
    widget_offset_x = coords.x1;  // Tọa độ x của góc trên bên trái
    widget_offset_y = coords.y1;  // Tọa độ y của góc trên bên trái
    
    return img;
}


// Callback giải mã của TJpg_Decoder
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    // Tính toán vị trí thực tế trên màn hình
    int16_t screen_x = x - crop_offset_x + widget_offset_x;
    int16_t screen_y = y - crop_offset_y + widget_offset_y;

    // Chỉ vẽ nếu vùng ảnh nằm trong phạm vi widget
    if (screen_x >= crop_offset_x && screen_y >= crop_offset_y &&
        screen_x < crop_offset_x + 200 && screen_y < crop_offset_y + 200) {
        tft.drawRGBBitmap(screen_x, screen_y, bitmap, w, h);
    }
    return 1;
}
// Hàm update_camera_widget dùng TJpg_Decoder để vẽ ảnh JPEG từ camera
void update_camera_widget(lv_obj_t* img) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    
    // Thêm kiểm tra hợp lệ cho buffer
    if (fb->buf != NULL && fb->len > 0) {
        TJpgDec.drawJpg(crop_offset_x, crop_offset_y, fb->buf, fb->len);
    }
    
    // Đảm bảo luôn giải phóng buffer
    esp_camera_fb_return(fb);
}



