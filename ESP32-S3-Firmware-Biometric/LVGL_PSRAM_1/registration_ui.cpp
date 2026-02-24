#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "lvgl.h"
#include "lvgl_ui.h"
#include "fingerprint_ui.h"
#include "http_client.h"
#include "registration_ui.h"
#include <base64.h>
#include <esp_camera.h>
#include "globals.h"


// Biến giao diện
lv_obj_t* regScreen = NULL;
lv_obj_t* regProgressLabel = NULL;
lv_obj_t* regStatusLabel = NULL;
lv_obj_t* regRegisterBtn = NULL;
lv_obj_t* regCancelBtn = NULL;

// Biến toàn cục
uint32_t currentMSSV = 0;
static FingerprintUIState currentState = FP_STATE_IDLE;

// ==== HÀM HỖ TRỢ CHUNG ====

void set_register_button_action(const char* labelText, lv_event_cb_t cb) {
    if (!regRegisterBtn) return;

    lv_obj_clear_flag(regRegisterBtn, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* label = lv_obj_get_child(regRegisterBtn, 0);
    if (label) lv_label_set_text(label, labelText);

    lv_obj_add_event_cb(regRegisterBtn, cb, LV_EVENT_CLICKED, NULL);
}

// ==== UI ====

void create_registration_screen_ui(uint32_t mssv) {
    regScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(regScreen, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t* title = lv_label_create(regScreen);
    lv_label_set_text_fmt(title, "Dang ky MSSV: %d", mssv);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    regProgressLabel = lv_label_create(regScreen);
    lv_label_set_text(regProgressLabel, "Kiem tra MSSV...");
    lv_obj_align(regProgressLabel, LV_ALIGN_TOP_LEFT, 10, 50);

    regStatusLabel = lv_label_create(regScreen);
    lv_label_set_text(regStatusLabel, "Dang xu ly...");
    lv_obj_set_style_text_color(regStatusLabel, lv_color_hex(0x003366), 0);
    lv_obj_align(regStatusLabel, LV_ALIGN_TOP_LEFT, 10, 110);

    regRegisterBtn = lv_btn_create(regScreen);
    lv_obj_set_size(regRegisterBtn, 120, 40);
    lv_obj_align(regRegisterBtn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_flag(regRegisterBtn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t* label = lv_label_create(regRegisterBtn);
    lv_label_set_text(label, "Dang ky");
    lv_obj_center(label);

    regCancelBtn = lv_btn_create(regScreen);
    lv_obj_set_size(regCancelBtn, 100, 40);
    lv_obj_align(regCancelBtn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_t* label2 = lv_label_create(regCancelBtn);
    lv_label_set_text(label2, "Huỷ");
    lv_obj_center(label2);

    lv_obj_add_event_cb(regCancelBtn, [](lv_event_t* e) {
        stop_camera_stream_server();
        camera_streaming = false;
        lv_scr_load(mainScreen);
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(regScreen);
}

void update_registration_ui(const char* progress, const char* status, bool showRegisterBtn) {
    if (regProgressLabel) lv_label_set_text(regProgressLabel, progress);
    if (regStatusLabel) lv_label_set_text(regStatusLabel, status);

    if (regRegisterBtn) {
        if (showRegisterBtn) lv_obj_clear_flag(regRegisterBtn, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(regRegisterBtn, LV_OBJ_FLAG_HIDDEN);
    }
}

// ==== Bước 1 ====

void start_registration_flow() {
    updateServerConfig(serverIP, serverPort);
    create_fingerprint_screen(FP_MODE_ENROLL_ONLY);
    switch_page(mainScreen, fingerprintScreen);

    show_fingerprint_numpad("Nhap MSSV", [](lv_event_t *e) {
        if (selectedID < 10000000 || selectedID > 99999999) {
            update_fingerprint_status(-1, "MSSV khong hop le");
            return;
        }

        currentMSSV = selectedID;
        create_registration_screen_ui(currentMSSV);
        update_registration_ui("Kiem tra MSSV...", "Dang kiem tra trang thai...", false);
        check_student_status(currentMSSV);
    });
}

// ==== Bước 2 ====

void check_student_status(uint32_t mssv) {
    updateServerConfig(serverIP, serverPort);
    String url = serverURL + "/api/student-status?id=" + String(mssv);

    update_registration_ui("Ket noi server...", "Dang gui yeu cau...", false);

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != 200) {
        update_registration_ui("Loi ket noi", "Khong the ket noi den server", false);
        set_register_button_action("Thu lai", [](lv_event_t* e) {
            check_student_status(currentMSSV);
        });
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        update_registration_ui("Loi du lieu", "Khong doc duoc JSON", false);
        set_register_button_action("Thu lai", [](lv_event_t* e) {
            check_student_status(currentMSSV);
        });
        return;
    }

    bool hasFace = doc["hasFace"].as<bool>();
    bool hasFingerprint = doc["hasFingerprint"].as<bool>();

    if (hasFace && hasFingerprint) {
        update_registration_ui("Da Dang ky Day Du", "Khong can Dang ky lai", true);
        lv_timer_create([](lv_timer_t* t) {
            if (mainScreen != NULL) {
                lv_scr_load(mainScreen);
            }
            lv_timer_del(t);  // Xoá timer sau khi chạy
        }, 2000, NULL);
    } else if (!hasFace) {
        update_registration_ui("Chua co khuon mat", "Chuan bi chup anh...", false);
        start_face_registration(mssv);
    } else {
        camera_streaming = false;
        stop_camera_stream_server();
        
        // Kiểm tra xem MSSV đã có vân tay chưa
        if (fingerprint.getSensorIDFromMSSV(mssv) != 0) {
            update_registration_ui("Da co van tay", "MSSV nay da dang ky van tay", true);
            lv_timer_create([](lv_timer_t* t) {
                if (mainScreen != NULL) {
                    lv_scr_load(mainScreen);
                }
                lv_timer_del(t);
            }, 2000, NULL);
            return;
        }
        
        update_registration_ui("Da co khuon mat\nDang cho dang ky van tay", "Dat ngon tay de dang ky...", false);
        if (fingerprintScreen == NULL) {
          Serial.println("⚠️ fingerprintScreen chưa được tạo!");
          return;
        }

        // create_fingerprint_screen(FP_MODE_ENROLL_ONLY);
        switch_page(regScreen, fingerprintScreen);

        String msg = "Dang ky MSSV " + String(mssv);
        update_fingerprint_status(0, msg.c_str());
        start_fingerprint_enrollment_direct();
    }
}

// ==== Bước 3: Camera ====

void start_face_registration(uint32_t mssv) {
    currentMSSV = mssv;

    switch_page(regScreen, evScreen);

    camera_streaming = true;
    delay(300);  // Đợi một chút để tránh lỗi bộ nhớ
    start_camera_stream_server();
    delay(1000);

    extern lv_timer_t *camera_update_timer;
    if (camera_update_timer) lv_timer_resume(camera_update_timer);

    lv_obj_t* infoLabel = lv_label_create(evScreen);
    lv_label_set_text(infoLabel, "Vui long nhin thang vao camera");
    lv_obj_set_style_text_color(infoLabel, lv_color_hex(0x003366), 0);
    lv_obj_set_style_text_font(infoLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(infoLabel, LV_ALIGN_TOP_MID, 0, 10);

    send_register_signal_to_web(currentMSSV);

}

// ==== Bước 4: Đăng ký vân tay ====

void start_fingerprint_enrollment_direct() {
    camera_streaming = false;
    stop_camera_stream_server();
    currentState = FP_STATE_ENROLLING;

    if (fpEnrollBtn) lv_obj_add_flag(fpEnrollBtn, LV_OBJ_FLAG_HIDDEN);
    if (fpVerifyBtn) lv_obj_add_flag(fpVerifyBtn, LV_OBJ_FLAG_HIDDEN);
    if (fpDeleteBtn) lv_obj_add_flag(fpDeleteBtn, LV_OBJ_FLAG_HIDDEN);

    if (fpActionBtn) lv_obj_clear_flag(fpActionBtn, LV_OBJ_FLAG_HIDDEN);

    if (!fingerprintScreen) {
        Serial.println("❌ fingerprintScreen tạo không thành công!");
        return;
    }
    Serial.printf(">> fingerprintScreen addr = 0x%08X\n", (uint32_t)fingerprintScreen);

    switch_page(regScreen, fingerprintScreen);

    String msg = "Dat ngon tay dang ky MSSV " + String(currentMSSV);
    update_fingerprint_status(0, msg.c_str());

    // Xóa timer cũ nếu tồn tại
    if (fpTimer) {
        lv_timer_del(fpTimer);
        fpTimer = NULL;
    }

    // Tạo timer mới với delay ngắn để tránh blocking
    fpTimer = lv_timer_create([](lv_timer_t *timer) {
        // Kiểm tra heap trước khi thực hiện
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 50000) {  // Nếu heap thấp hơn 50KB
            Serial.printf("⚠️ Heap thấp: %d bytes, hoãn đăng ký\n", freeHeap);
            return;  // Không xóa timer, sẽ thử lại
        }
        
        fingerprint.enrollFingerprint(currentMSSV, enrollment_callback);
        
        // Xóa timer sau khi hoàn thành
        if (fpTimer) {
            lv_timer_del(fpTimer);
            fpTimer = NULL;
        }
    }, 500, NULL);
}
