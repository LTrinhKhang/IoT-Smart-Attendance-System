#include "attendance_flow.h"
#include "fingerprint_ui.h"    // chứa start_fingerprint_verification(...)
#include "Finger.h"                 // chứa decode_base64, get_template_by_mssv, ...
#include "lvgl_ui.h"            // chứa mainScreen, evScreen, switch_page(...)
#include "camera_stream.h"    // chứa start_camera_stream_server, stop_camera_stream_server
#include "lvgl_setup.h"      // chứa lv_timer_t, lv_obj_t, lv_label_create, ...
#include "http_client.h"
#include "fingerprint_storage.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Giả lập biến toàn cục
extern String studentID;
extern String studentName;
extern bool faceRecognized;
extern bool camera_streaming;
bool alreadyVerified = false;
extern String studentClass;  // biến toàn cục

lv_timer_t* returnToMainTimer = NULL;

String removeVietnameseTones(String str) {
  const char* from[] = {
    "à", "á", "ạ", "ả", "ã", "â", "ầ", "ấ", "ậ", "ẩ", "ẫ", "ă", "ằ", "ắ", "ặ", "ẳ", "ẵ",
    "è", "é", "ẹ", "ẻ", "ẽ", "ê", "ề", "ế", "ệ", "ể", "ễ",
    "ì", "í", "ị", "ỉ", "ĩ",
    "ò", "ó", "ọ", "ỏ", "õ", "ô", "ồ", "ố", "ộ", "ổ", "ỗ", "ơ", "ờ", "ớ", "ợ", "ở", "ỡ",
    "ù", "ú", "ụ", "ủ", "ũ", "ư", "ừ", "ứ", "ự", "ử", "ữ",
    "ỳ", "ý", "ỵ", "ỷ", "ỹ",
    "đ",
    "À", "Á", "Ạ", "Ả", "Ã", "Â", "Ầ", "Ấ", "Ậ", "Ẩ", "Ẫ", "Ă", "Ằ", "Ắ", "Ặ", "Ẳ", "Ẵ",
    "È", "É", "Ẹ", "Ẻ", "Ẽ", "Ê", "Ề", "Ế", "Ệ", "Ể", "Ễ",
    "Ì", "Í", "Ị", "Ỉ", "Ĩ",
    "Ò", "Ó", "Ọ", "Ỏ", "Õ", "Ô", "Ồ", "Ố", "Ộ", "Ổ", "Ỗ", "Ơ", "Ờ", "Ớ", "Ợ", "Ở", "Ỡ",
    "Ù", "Ú", "Ụ", "Ủ", "Ũ", "Ư", "Ừ", "Ứ", "Ự", "Ử", "Ữ",
    "Ỳ", "Ý", "Ỵ", "Ỷ", "Ỹ",
    "Đ"
  };

  const char* to[] = {
    "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a",
    "e", "e", "e", "e", "e", "e", "e", "e", "e", "e", "e",
    "i", "i", "i", "i", "i",
    "o", "o", "o", "o", "o", "o", "o", "o", "o", "o", "o", "o", "o", "o", "o", "o", "o",
    "u", "u", "u", "u", "u", "u", "u", "u", "u", "u", "u",
    "y", "y", "y", "y", "y",
    "d",
    "A", "A", "A", "A", "A", "A", "A", "A", "A", "A", "A", "A", "A", "A", "A", "A", "A",
    "E", "E", "E", "E", "E", "E", "E", "E", "E", "E", "E",
    "I", "I", "I", "I", "I",
    "O", "O", "O", "O", "O", "O", "O", "O", "O", "O", "O", "O", "O", "O", "O", "O", "O",
    "U", "U", "U", "U", "U", "U", "U", "U", "U", "U", "U",
    "Y", "Y", "Y", "Y", "Y",
    "D"
  };

  for (int i = 0; i < sizeof(from)/sizeof(from[0]); i++) {
    str.replace(from[i], to[i]);
  }
  return str;
}


// ==== Bước 1: Bắt đầu luồng điểm danh ====
void start_attendance_flow() {
    switch_page(mainScreen, evScreen);
    start_camera_stream_server();
    delay(1000);
    camera_streaming = true;

    extern lv_timer_t *camera_update_timer;
    if (camera_update_timer) lv_timer_resume(camera_update_timer);

    lv_obj_t* infoLabel = lv_label_create(evScreen);
    lv_label_set_text(infoLabel, "Vui long nhin thang vao camera");
    lv_obj_set_style_text_color(infoLabel, lv_color_hex(0x003366), 0);
    lv_obj_set_style_text_font(infoLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(infoLabel, LV_ALIGN_TOP_MID, 0, 10);

    Serial.println("🟢 Đang chờ kết quả nhận diện khuôn mặt từ web...");
    
    // 🔥 Gửi tín hiệu điểm danh đến web server
    send_attendance_signal_to_web();
}

// ==== Bước 2: Khi web gửi kết quả nhận diện ====
void on_face_result_received(bool success, const String& mssv, const String& name, const String& className){
    faceRecognized = success;
    studentID = mssv;
    studentName = name;
    camera_streaming = false;

    if (!success) {
        static lv_obj_t* failLabel = NULL;

        if (failLabel != NULL) lv_obj_del(failLabel);
        failLabel = lv_label_create(evScreen);
        lv_label_set_text(failLabel, "Không nhận diện được khuôn mặt.");
        lv_obj_align(failLabel, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    // 🔍 Kiểm tra MSSV đã đăng ký vân tay chưa
    if (!fingerprint.isMSSVRegistered(mssv.toInt())) {
        Serial.println("❌ MSSV chưa đăng ký vân tay. Hủy xác thực.");

        update_fingerprint_status(-1, "❌ MSSV chua co du lieu van tay!");
        return;
    }

    // ✅ Đã có vân tay → chuyển sang xác thực với MSSV cụ thể với MSSV cụ thể
    create_fingerprint_screen(FP_MODE_VERIFY);
    switch_page(evScreen, fingerprintScreen);
    String nameNoAccent = removeVietnameseTones(name);
    update_fingerprint_status(0, ("Xin chao " + mssv + " " + nameNoAccent + "\nDiem danh bang van tay...").c_str());
    alreadyVerified = false;  // reset lại khi nhận kết quả từ face
    stop_camera_stream_server();
    camera_streaming = false;

    // 🔐 Bắt đầu xác thực vân tay với MSSV cụ thể
    start_fingerprint_mssv_verification(mssv.toInt(), [](bool matched) {
    if (alreadyVerified) {
        Serial.println("⚠️ Callback đã xử lý trước đó, bỏ qua...");
        return;
    }
    alreadyVerified = true;

    Serial.printf("📌 Callback xác thực gọi: matched = %s\n", matched ? "true" : "false");

    String mssvStr = studentID;
    String nameStr = studentName;
    String classStr = studentClass;

    if (matched) {
        update_fingerprint_status(1, "✅ Vân tay khớp MSSV");
        record_attendance(mssvStr);
        show_result_screen(true, mssvStr, nameStr, classStr);
    } else {
        update_fingerprint_status(-1, "❌ Vân tay không khớp MSSV");
        show_result_screen(false, mssvStr, nameStr, classStr);
    }
});
}



// ==== Hàm mới: Xử lý khi web gửi kết quả điểm danh thành công ====
void on_attendance_success_received(const String& name) {
    String nameNoAccent = removeVietnameseTones(name);
    Serial.println("✅ Web da diem danh thanh cong cho: " + nameNoAccent);
    
    // Hiển thị màn hình thành công
    show_result_screen(true, "Diem danh thanh cong!\n");
    
    // Dừng camera stream
    stop_camera_stream_server();
    camera_streaming = false;
}

// ==== Bước 4: Kết quả xác thực ====
void on_fingerprint_verified(bool matched, const String& mssv, const String& name) {
    if (matched) {
        record_attendance(mssv);
        show_result_screen(true, ("Diem danh thanh cong!\n" + name).c_str());
    } else {
        show_result_screen(false, "Van tay khong dung.");
    }
}

// ==== Bước 5: Gửi kết quả về server ====
void record_attendance(const String& mssv) {
    HTTPClient http;
    http.begin(serverURL + "/api/attendance");
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["mssv"] = mssv;
    doc["time"] = getCurrentTimestamp();

    String body;
    serializeJson(doc, body);
    http.POST(body);
    http.end();
}

// ==== Bước 6: Giao diện kết quả ====
void show_result_screen(bool success, const String& mssv, const String& name, const String& className) {
    // 1. Huỷ timer tự động (nếu đang dùng)
    if (returnToMainTimer) {
        lv_timer_del(returnToMainTimer);
        returnToMainTimer = NULL;
    }
    

    // 2. Tạo màn hình kết quả
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, success ? lv_color_hex(0xCCFFCC) : lv_color_hex(0xFFCCCC), 0);

    // 3. Font và khoảng cách
    lv_coord_t top_offset = 30;
    lv_coord_t spacing = 40;

    // 4. Hiển thị MSSV
    lv_obj_t* mssvLabel = lv_label_create(screen);
    lv_label_set_text_fmt(mssvLabel, "MSSV: %s", mssv.c_str());
    lv_obj_set_style_text_font(mssvLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(mssvLabel, LV_ALIGN_TOP_LEFT, 0, top_offset);

    // 5. Hiển thị Họ tên
    lv_obj_t* nameLabel = lv_label_create(screen);
    String noAccentName = removeVietnameseTones(name);
    lv_label_set_text_fmt(nameLabel, "Name: %s", noAccentName.c_str());
    lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(nameLabel, LV_ALIGN_TOP_LEFT, 0, top_offset + spacing);

    // 6. Hiển thị Lớp
    lv_obj_t* classLabel = lv_label_create(screen);
    String noAccentClass = removeVietnameseTones(className);
    lv_label_set_text_fmt(classLabel, "Class: %s", noAccentClass.c_str());
    lv_obj_set_style_text_font(classLabel, &lv_font_montserrat_18, 0);
    lv_obj_align(classLabel, LV_ALIGN_TOP_LEFT, 0, top_offset + spacing * 2);

    // 7. Thông báo trạng thái điểm danh
    lv_obj_t* statusLabel = lv_label_create(screen);
    lv_label_set_text(statusLabel, success ? "Diem danh thanh cong" : "Diem danh that bai");
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_20, 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 0, top_offset + spacing * 3);

    // 8. Nút quay về
    lv_obj_t* btn = lv_btn_create(screen);
    lv_obj_set_size(btn, 120, 40);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Home");
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        lv_scr_load(mainScreen);
    }, LV_EVENT_CLICKED, NULL);

    // 9. Hiển thị toàn bộ giao diện
    lv_scr_load(screen);
}

