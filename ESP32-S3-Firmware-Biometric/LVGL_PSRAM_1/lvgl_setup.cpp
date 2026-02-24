#include "lvgl_setup.h"
#include "display_touch.h"
#include <Arduino.h>

// Định nghĩa biến toàn cục
lv_color_t *buf1 = NULL;
lv_color_t *buf2 = NULL;
lv_disp_draw_buf_t draw_buf;
lv_disp_drv_t disp_drv;

// Biến để lưu kích thước buffer thực tế đã cấp phát (tính bằng số lượng pixel)
static size_t actual_buffer_size = 0; 

void init_lvgl_buffers() {
    // Reset kích thước buffer thực tế
    actual_buffer_size = 0;

    // Kiểm tra PSRAM trước khi cấp phát
    if (psramFound()) {
        Serial.println("PSRAM được tìm thấy và sẵn sàng sử dụng");
        
        // Cấp phát bộ nhớ từ PSRAM
        buf1 = (lv_color_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
        if (!buf1) {
            Serial.println("Lỗi cấp phát buffer1!");
        } else {
            buf2 = (lv_color_t *)heap_caps_malloc(BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
            if (!buf2) {
                Serial.println("Lỗi cấp phát buffer2!");
                // Giải phóng buffer1 nếu đã cấp phát thành công
                heap_caps_free(buf1);
                buf1 = NULL;
            }
        }
        
        if (buf1 && buf2) {
            Serial.println("Đã cấp phát thành công 2 buffer LVGL trong PSRAM!");
            actual_buffer_size = BUFFER_SIZE; // Lưu kích thước thực tế
            Serial.printf("Kích thước mỗi buffer: %u bytes\n", actual_buffer_size * sizeof(lv_color_t));
        }
    } 
    
    // Nếu cấp phát PSRAM thất bại hoặc không có PSRAM
    if (!buf1 || !buf2) {
        if (psramFound()) {
             Serial.println("Lỗi cấp phát bộ nhớ PSRAM, thử dùng heap thông thường...");
        } else {
            Serial.println("PSRAM không được tìm thấy, sử dụng heap thông thường");
        }
        
        // Thử cấp phát từ heap thông thường với kích thước nhỏ hơn
        // Giảm kích thước đáng kể cho heap, ví dụ / 4 hoặc / 8
        size_t smaller_size = BUFFER_SIZE / 4; 
        buf1 = (lv_color_t *)malloc(smaller_size * sizeof(lv_color_t));
        buf2 = (lv_color_t *)malloc(smaller_size * sizeof(lv_color_t));
        
        if (buf1 && buf2) {
            actual_buffer_size = smaller_size; // Lưu kích thước thực tế
            Serial.printf("Đã cấp phát 2 buffer nhỏ hơn (%u bytes) từ heap thông thường\n", 
                         actual_buffer_size * sizeof(lv_color_t));
        } else {
            Serial.println("Không thể cấp phát bộ nhớ cho LVGL!");
            // Giải phóng nếu chỉ cấp phát được 1 buffer
            if(buf1) free(buf1);
            if(buf2) free(buf2);
            buf1 = NULL;
            buf2 = NULL;
            ESP.restart(); // Khởi động lại nếu không thể cấp phát bộ nhớ
        }
    }
}

void init_lvgl() {
    // Kiểm tra xem buffer đã được cấp phát chưa
    if (!buf1 || !buf2 || actual_buffer_size == 0) {
        Serial.println("Lỗi: Buffer LVGL chưa được cấp phát! Không thể khởi tạo LVGL.");
        return; // Hoặc xử lý lỗi khác, ví dụ ESP.restart()
    }

    lv_init();
    Serial.println("LVGL đã khởi động!");
    
    // *** SỬ DỤNG KÍCH THƯỚC THỰC TẾ ĐÃ CẤP PHÁT ***
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, actual_buffer_size); 
    
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_drv_register(&indev_drv);
    
    Serial.println("LVGL đã sẵn sàng!");
}
