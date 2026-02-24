#include "lvgl_ui.h"
#include "wifi_ntp.h"
#include "display_touch.h"
#include "lvgl_setup.h"
#include "camera_stream.h"
#include "Finger.h"
#include "fingerprint_storage.h"
#include "wifiScreen.h"
#include <TJpg_Decoder.h>  // Đảm bảo đã include thư viện TJpg_Decoder
#include "http_client.h"
#include "fingerprint_ui.h"

// Biến toàn cục để theo dõi timer
lv_timer_t *camera_update_timer = NULL;

// Hàm timer cập nhật camera stream
void camera_timer_cb(lv_timer_t * timer) {
    if (camera_streaming && camera_img) {
        update_camera_widget(camera_img);
    } else if (!camera_streaming && timer) {
        // Nếu không còn streaming nhưng timer vẫn hoạt động, tạm dừng nó
        lv_timer_pause(timer);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Khởi động ESP32-S3...");

    Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM (Before Allocation): %d bytes\n", ESP.getFreePsram());
    
    // Khởi tạo module vân tay
    if (fingerprint.begin()) {
        Serial.println("Cảm biến vân tay đã sẵn sàng!");
    } else {
        Serial.println("Không thể kết nối với cảm biến vân tay!");
    }

    init_lvgl_buffers();
    
    // Khởi tạo WiFi
    Serial.println("Khởi tạo WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);  // Xóa cấu hình cũ
    delay(500);  // Tăng delay để đảm bảo WiFi reset hoàn toàn

    init_display();
    init_touch();
    TJpgDec.setCallback(tft_output);
    init_camera();
    init_lvgl();
    create_wifi_screen();          // Tạo màn hình WiFi
    lv_scr_load(wifiScreen);


    // Tạo task stream riêng biệt ở core 1
    xTaskCreatePinnedToCore(
      stream_task,        // Hàm task chạy nền
      "Camera Stream",    // Tên
      10000,               // Stack size
      NULL,               // Tham số
      1,                  // Ưu tiên
      NULL,               // Handle
      0                   // Core 1
    );

    // Tạo timer LVGL để cập nhật stream từ camera mỗi 100ms
    camera_update_timer = lv_timer_create(camera_timer_cb, 100, NULL);
    // Tạm dừng timer ban đầu vì chưa cần streaming
    lv_timer_pause(camera_update_timer);
    
    // Hiển thị số lượng mẫu vân tay đã lưu
    uint16_t templateCount = fingerprint.getTemplateCount();
    Serial.printf("Số lượng mẫu vân tay đã lưu: %d\n", templateCount);

    Serial.printf("Free heap: %d bytes\n", esp_get_free_heap_size());
}

// Biến để theo dõi bộ nhớ
static unsigned long lastMemCheck = 0;
static uint32_t lastFreeHeap = 0;
static uint32_t lastFreePsram = 0;

void loop() {
    
    lv_task_handler();

    if (isStreamActive && camera_streaming) {
        camera_stream_server_handle();
    }
    if (shouldDeleteFpTimer && fpTimer) {
      lv_timer_del(fpTimer);
      fpTimer = NULL;
      shouldDeleteFpTimer = false;
      Serial.println("fpTimer deleted safely in loop");
    }
    // Kiểm tra bộ nhớ mỗi 10 giây
    unsigned long currentMillis = millis();
    if (currentMillis - lastMemCheck > 10000) {
        uint32_t freeHeap = esp_get_free_heap_size();
        uint32_t freePsram = ESP.getFreePsram();
        
        // Kiểm tra stack usage
        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        
        // Chỉ in ra nếu có thay đổi đáng kể (>5KB) hoặc stack thấp
        if (abs((int32_t)freeHeap - (int32_t)lastFreeHeap) > 5000 || 
            abs((int32_t)freePsram - (int32_t)lastFreePsram) > 5000 ||
            stackHighWaterMark < 1000) {  // Cảnh báo nếu stack còn < 1KB
            Serial.printf("Free heap: %d bytes, Free PSRAM: %d bytes, Stack free: %d bytes\n", 
                         freeHeap, freePsram, stackHighWaterMark * sizeof(StackType_t));
            
            if (stackHighWaterMark < 1000) {
                Serial.println("CẢNH BÁO: Stack sắp hết!");
            }
            
            // Nếu heap quá thấp, thực hiện các biện pháp khẩn cấp
            if (freeHeap < 20000) {
                Serial.println("⚠️ HEAP THẤP - Thực hiện dọn dẹp khẩn cấp");
                
                // Dừng camera stream nếu đang chạy
                if (camera_streaming) {
                    camera_streaming = false;
                    stop_camera_stream_server();
                    Serial.println("🛑 Đã dừng camera stream để tiết kiệm bộ nhớ");
                }
                
                // Force garbage collection
                esp_restart();  // Restart nếu heap quá thấp
            }
            
            lastFreeHeap = freeHeap;
            lastFreePsram = freePsram;
        }
        
        lastMemCheck = currentMillis;
    }
    
    delay(5);
}