#include <WiFi.h>
#include "wifiScreen.h"
#include "lvgl_setup.h"
#include "lvgl_ui.h"
#include "wifi_ntp.h"
#include "http_client.h"

lv_obj_t *wifiScreen = NULL;            // Màn hình chính của ứng dụng
lv_obj_t *wifiStatusLabel = NULL;       // Nhãn hiển thị trạng thái WiFi
lv_obj_t *wifiList = NULL;              // Danh sách các mạng WiFi
lv_obj_t *wifiConnectBtn = NULL;        // Nút kết nối WiFi
lv_obj_t *wifiDisconnectBtn = NULL;     // Nút ngắt kết nối WiFi
lv_obj_t *wifiPasswordInput = NULL;     // Trường nhập mật khẩu WiFi
lv_obj_t *keyboard = NULL;              // Bàn phím ảo cho nhập mật khẩu WiFi
lv_obj_t *wifiReloadBtn = NULL;         // Nút quét lại mạng WiFi
lv_timer_t *wifiCheckTimer = NULL;      // Timer kiểm tra kết nối WiFi
lv_timer_t *wifiScanTimer = NULL;       // Timer kiểm tra quét WiFi

static char selectedSSID[32] = {0};     // Biến lưu SSID đang chọn
static int connectionAttempts = 0;      // Đếm số lần thử kết nối
static const int MAX_CONNECTION_ATTEMPTS = 20; // Timeout 10 giây (20 * 500ms)
bool wifiFlag = false;


// Hàm kiểm tra trạng thái kết nối WiFi
void check_wifi_status_cb(lv_timer_t *timer) {
    connectionAttempts++;
    
    if (WiFi.status() == WL_CONNECTED) {
        lv_label_set_text(wifiStatusLabel, "Ket noi thanh cong");
        Serial.println("WiFi kết nối thành công!");
        wifiFlag = true;
        // Reset counter và dừng timer
        connectionAttempts = 0;
        lv_timer_del(timer);
        wifiCheckTimer = NULL;

        // Delay một chút để đảm bảo WiFi ổn định
        delay(500);
        if (wifiFlag == true) {
          camera_streaming = false;  // Tắt streaming khi chuyển trang
          cleanup_wifi_screen();
          create_ui();
          initNTP();
          delay(1000); 
          update_time();
          
          
          Serial.println("Đã đồng bộ thời gian từ NTP!");
          Serial.println("Tạo giao diện chính...");
          Serial.println("WiFi OK - Đã chuyển sang giao diện chính!");
          initHTTPClient();
          updateServerConfig(serverIP,serverPort );

          delay(50);  // Tăng delay để đảm bảo UI được tạo hoàn toàn
          lv_scr_load(mainScreen);
          printConnectionInfo();
          
          
        }
        
        
    } 
    else if (connectionAttempts >= MAX_CONNECTION_ATTEMPTS) {
        // Timeout - kết nối thất bại
        wifiFlag = false;
        lv_label_set_text(wifiStatusLabel, "Connection failed - Timeout");
        Serial.println("WiFi connection timeout!");
        
        // Reset và dừng timer
        connectionAttempts = 0;
        lv_timer_del(timer);
        wifiCheckTimer = NULL;
        
        // Ngắt kết nối
        WiFi.disconnect();
    }
    else {
        // Hiển thị tiến trình
        lv_label_set_text_fmt(wifiStatusLabel, "Connecting... (%d/%d)", 
                              connectionAttempts, MAX_CONNECTION_ATTEMPTS);
        Serial.printf("WiFi connecting attempt %d/%d, status: %d\n", 
                     connectionAttempts, MAX_CONNECTION_ATTEMPTS, WiFi.status());
    }
}

// Hàm xử lý sự kiện khi nhấn nút mạng WiFi trong danh sách
void wifi_list_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *btn = lv_event_get_target(e);
        
        // Kiểm tra button có hợp lệ không
        if (!btn || !wifiList) {
            Serial.println("Lỗi: Button hoặc wifiList NULL");
            return;
        }
        
        const char *ssid = lv_list_get_btn_text(wifiList, btn);
        
        Serial.printf("Event click - btn: %p, ssid: %s\n", btn, ssid ? ssid : "NULL");
        
        if (ssid && strlen(ssid) > 0 && strlen(ssid) < sizeof(selectedSSID)) {
            // Sao chép SSID an toàn
            memset(selectedSSID, 0, sizeof(selectedSSID));
            strncpy(selectedSSID, ssid, sizeof(selectedSSID) - 1);
            selectedSSID[sizeof(selectedSSID) - 1] = '\0';
            
            lv_label_set_text(wifiStatusLabel, selectedSSID);
            Serial.printf("Đã chọn SSID: %s\n", selectedSSID);

            // Hiện trường nhập mật khẩu, bàn phím và nút kết nối khi chọn xong SSID
            if (wifiPasswordInput) lv_obj_clear_flag(wifiPasswordInput, LV_OBJ_FLAG_HIDDEN);
            if (keyboard) lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
            if (wifiConnectBtn) lv_obj_clear_flag(wifiConnectBtn, LV_OBJ_FLAG_HIDDEN);
            if (wifiDisconnectBtn) lv_obj_clear_flag(wifiDisconnectBtn, LV_OBJ_FLAG_HIDDEN);
        } else {
            Serial.println("Lỗi: SSID NULL, rỗng hoặc quá dài");
        }
    }
}




// Hàm timer kiểm tra trạng thái quét WiFi không đồng bộ
static void wifi_scan_timer_cb(lv_timer_t *timer) {
    int scanStatus = WiFi.scanComplete();
    // Serial.printf("Scan status: %d\n", scanStatus);
    
    if (scanStatus >= 0) {
        Serial.printf("Tìm thấy %d mạng WiFi\n", scanStatus);
        lv_obj_clean(wifiList); // Xóa danh sách cũ

        if (scanStatus == 0) {
            lv_label_set_text(wifiStatusLabel, "WiFi network not found");
            Serial.println("Không có mạng WiFi nào được tìm thấy");
        } else {
            for (int i = 0; i < scanStatus; i++) {
                String ssid = WiFi.SSID(i);
                int32_t rssi = WiFi.RSSI(i);
                wifi_auth_mode_t encryption = WiFi.encryptionType(i);
                
                Serial.printf("Mạng %d: SSID='%s', RSSI=%d, Encryption=%d\n", 
                             i, ssid.c_str(), rssi, encryption);
                
                // Kiểm tra SSID có hợp lệ không
                if (ssid.length() > 0 && !ssid.equals("")) {
                    lv_obj_t *btn = lv_list_add_btn(wifiList, LV_SYMBOL_WIFI, ssid.c_str());
                    if (btn) {
                        lv_obj_add_event_cb(btn, wifi_list_event_cb, LV_EVENT_CLICKED, NULL);
                        Serial.printf("Đã thêm nút cho SSID: %s (btn: %p)\n", ssid.c_str(), btn);
                        
                        // Kiểm tra xem text có được set đúng không
                        const char* btn_text = lv_list_get_btn_text(wifiList, btn);
                        Serial.printf("Text của nút vừa tạo: %s\n", btn_text ? btn_text : "NULL");
                    } else {
                        Serial.printf("Lỗi: Không thể tạo nút cho SSID: %s\n", ssid.c_str());
                    }
                } else {
                    Serial.printf("Bỏ qua SSID rỗng hoặc không hợp lệ ở index %d\n", i);
                }
            }
            
            // Kiểm tra số lượng item trong list
            uint32_t child_count = lv_obj_get_child_cnt(wifiList);
            Serial.printf("Số lượng item trong wifiList: %d\n", child_count);
            
            lv_label_set_text(wifiStatusLabel, "Select WiFi network");
        }

        WiFi.scanDelete();    // Xóa bộ nhớ scan cũ
        lv_timer_del(timer);  // Dừng timer quét
        wifiScanTimer = NULL;
        Serial.println("Hoàn thành quét WiFi");
    } else if (scanStatus == WIFI_SCAN_RUNNING) {
        lv_label_set_text(wifiStatusLabel, "Scanning WiFi networks...");
        Serial.println("Đang quét WiFi...");
    } else {
        lv_label_set_text(wifiStatusLabel, "WiFi scanning error");
        Serial.printf("Lỗi quét WiFi: %d\n", scanStatus);
        lv_timer_del(timer);
        wifiScanTimer = NULL;
    }
}

// Hàm xử lý sự kiện khi nhấn nút "Reload Scan"
static void wifi_reload_scan_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // Dừng timer quét cũ nếu đang chạy
        if (wifiScanTimer) {
            lv_timer_del(wifiScanTimer);
            wifiScanTimer = NULL;
        }
        
        // Xóa danh sách WiFi cũ
        lv_obj_clean(wifiList);
        
        // Cập nhật trạng thái
        lv_label_set_text(wifiStatusLabel, "Scanning WiFi networks...");
        Serial.println("Bắt đầu quét lại mạng WiFi...");
        
        // Bắt đầu quét WiFi không đồng bộ
        WiFi.scanDelete();  // Xóa kết quả quét cũ
        int scanResult = WiFi.scanNetworks(true);
        Serial.printf("Kết quả khởi tạo scan: %d\n", scanResult);
        
        // Tạo timer kiểm tra trạng thái quét mỗi 500ms
        wifiScanTimer = lv_timer_create(wifi_scan_timer_cb, 500, NULL);
        Serial.println("Đã tạo timer quét WiFi mới");
    }
}




// Hàm tạo màn hình WiFi
void create_wifi_screen() {
    Serial.println("Bắt đầu tạo màn hình WiFi");
    
    // Tạo màn hình mới (không dùng màn hình hiện tại)
    wifiScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wifiScreen, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    // lv_scr_load(wifiScreen);

    // Nhãn trạng thái WiFi
    wifiStatusLabel = lv_label_create(wifiScreen);
    lv_label_set_text(wifiStatusLabel, "Initialize WiFi...");
    lv_obj_align(wifiStatusLabel, LV_ALIGN_TOP_MID, 0, 10);

    // Danh sách WiFi
    wifiList = lv_list_create(wifiScreen);
    lv_obj_set_size(wifiList, 280, 150);
    lv_obj_align(wifiList, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_bg_color(wifiList, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    // Kiểm tra trạng thái WiFi trước khi quét
    Serial.printf("WiFi mode: %d\n", WiFi.getMode());
    Serial.printf("WiFi status: %d\n", WiFi.status());
    
    // Đảm bảo WiFi ở chế độ Station
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Bắt đầu quét WiFi không đồng bộ
    Serial.println("Bắt đầu quét mạng WiFi...");
    int scanResult = WiFi.scanNetworks(true);
    Serial.printf("Kết quả khởi tạo scan: %d\n", scanResult);

    // Tạo timer kiểm tra trạng thái quét mỗi 500ms nếu chưa có
    if (!wifiScanTimer) {
        wifiScanTimer = lv_timer_create(wifi_scan_timer_cb, 500, NULL);
        Serial.println("Đã tạo timer quét WiFi");
    }

    // Trường nhập mật khẩu
    wifiPasswordInput = lv_textarea_create(wifiScreen);
    lv_textarea_set_placeholder_text(wifiPasswordInput, "Enter password");
    lv_textarea_set_password_mode(wifiPasswordInput, true);
    lv_obj_set_size(wifiPasswordInput, 200, 50);
    lv_obj_align(wifiPasswordInput, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_flag(wifiPasswordInput, LV_OBJ_FLAG_HIDDEN);  // Ẩn mật khẩu lúc đầu

    // Nút Reload Scan
    wifiReloadBtn = lv_btn_create(wifiScreen);
    lv_obj_align(wifiReloadBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t *label3 = lv_label_create(wifiReloadBtn);
    lv_label_set_text(label3, "Reload Scan");
    lv_obj_add_event_cb(wifiReloadBtn, wifi_reload_scan_event_cb, LV_EVENT_CLICKED, NULL);

    // Bàn phím ảo
    keyboard = lv_keyboard_create(wifiScreen);
    lv_keyboard_set_textarea(keyboard, wifiPasswordInput);
    lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);  // Ẩn bàn phím lúc đầu


    // Reset SSID đã chọn
    selectedSSID[0] = '\0';
}

// Hàm cleanup để giải phóng tài nguyên
void cleanup_wifi_screen() {
    // Dừng các timer nếu đang chạy
    if (wifiCheckTimer) {
        lv_timer_del(wifiCheckTimer);
        wifiCheckTimer = NULL;
    }
    
    if (wifiScanTimer) {
        lv_timer_del(wifiScanTimer);
        wifiScanTimer = NULL;
    }
    
    // Reset biến
    connectionAttempts = 0;
    selectedSSID[0] = '\0';
    
    // // Ngắt kết nối WiFi nếu đang kết nối
    // if (WiFi.status() == WL_CONNECTED) {
    //     WiFi.disconnect();
    // }
    
    Serial.println("WiFi screen cleanup completed");
}
// Hàm xử lý sự kiện bàn phím
void keyboard_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_target(e);
    lv_obj_t * ta = lv_keyboard_get_textarea(kb);

    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        // Khi nhấn nút Enter (READY) hoặc Cancel thì ẩn bàn phím
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        
        // Ẩn trường nhập mật khẩu và xóa nội dung
        if (wifiPasswordInput) {
            lv_obj_add_flag(wifiPasswordInput, LV_OBJ_FLAG_HIDDEN);
            if (code == LV_EVENT_READY) {
                // Lưu mật khẩu trước khi xóa
                const char *password = lv_textarea_get_text(ta);
                String savedPassword = String(password);
                
                // Xóa nội dung đã nhập
                lv_textarea_set_text(wifiPasswordInput, "");
                
                // Chỉ kết nối khi nhấn READY (Enter), không kết nối khi Cancel
                // Lấy SSID đã chọn từ biến toàn cục
                if (strlen(selectedSSID) == 0) {
                    lv_label_set_text(wifiStatusLabel, "Please select WiFi network");
                    return;
                }

                // Dừng timer cũ nếu có
                if (wifiCheckTimer) {
                    lv_timer_del(wifiCheckTimer);
                    wifiCheckTimer = NULL;
                }

                Serial.printf("Kết nối SSID: %s\n", selectedSSID); // Không in password

                // Ngắt kết nối cũ trước khi kết nối mới
                WiFi.disconnect();
                delay(100);
                
                WiFi.begin(selectedSSID, savedPassword.c_str());
                lv_label_set_text(wifiStatusLabel, "Connecting...");
                
                // Reset counter và tạo timer mới
                connectionAttempts = 0;
                wifiCheckTimer = lv_timer_create(check_wifi_status_cb, 500, NULL);
            }
        }
    }
}


