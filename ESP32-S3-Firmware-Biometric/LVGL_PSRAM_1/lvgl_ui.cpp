#include "lvgl_ui.h"
#include "lvgl_setup.h"
#include "camera_stream.h"  // include module camera
#include "wifiScreen.h"
#include "http_client.h"
#include "fingerprint_ui.h"
#include "registration_ui.h"
#include "attendance_flow.h"
#include "globals.h"





// Định nghĩa các biến giao diện
lv_obj_t *clockLabel = NULL;
lv_obj_t *dateLabel = NULL;
lv_obj_t *weatherLabel = NULL;
lv_obj_t *btnEV = NULL;
lv_obj_t *btnWifi = NULL;
lv_obj_t *btnRegister = NULL;
lv_obj_t *btnThermo = NULL;
lv_obj_t *btnFinger = NULL;  // Thêm nút vân tay
lv_obj_t *btnContainer = NULL;
lv_obj_t *mainScreen = NULL;
lv_obj_t *evScreen = NULL;
lv_obj_t *homeScreen = NULL;
lv_obj_t *meterScreen = NULL;
lv_obj_t *btnAttendance = NULL;

// Biến cho camera stream
lv_obj_t *camera_img = NULL;
bool camera_streaming = false;

// // Thêm biến cờ toàn cục
// bool shouldDeleteFpTimer = false;
static lv_timer_t *page_switch_timer = NULL;    // Biến toàn cục để theo dõi timer chuyển trang






void create_main_screen() {
  mainScreen = lv_obj_create(NULL);  // ✅ Tạo mới từ NULL, không dùng lv_scr_act()
  lv_obj_set_style_bg_color(mainScreen, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
  
}

void create_labels() {
  // 🔹 Ngày
  dateLabel = lv_label_create(mainScreen);
  lv_label_set_text(dateLabel, "Wednesday, 1");
  lv_obj_set_style_text_color(dateLabel, lv_color_hex(0x009966), LV_PART_MAIN);
  lv_obj_align(dateLabel, LV_ALIGN_TOP_LEFT, 20, 10);

  // 🔹 Đồng hồ
  clockLabel = lv_label_create(mainScreen);
  lv_obj_set_style_text_color(clockLabel, lv_color_hex(0x009966), LV_PART_MAIN);
  lv_obj_set_style_text_font(clockLabel, &lv_font_montserrat_48, LV_PART_MAIN);
  lv_label_set_text(clockLabel, "09:37");
  lv_obj_align(clockLabel, LV_ALIGN_TOP_LEFT, 20, 40);

  // 🔹 Thời tiết
  weatherLabel = lv_label_create(mainScreen);
  lv_label_set_text(weatherLabel, "16°C\nCloudy");
  lv_obj_set_style_text_color(weatherLabel, lv_color_hex(0x009966), LV_PART_MAIN);
  lv_obj_set_style_text_font(clockLabel, &lv_font_montserrat_30, LV_PART_MAIN);
  lv_obj_align(weatherLabel, LV_ALIGN_TOP_RIGHT, -20, 20);
}

void add_back_button(lv_obj_t *screen) {
  lv_obj_t *btnBack = lv_btn_create(screen);
  lv_obj_set_size(btnBack, 40, 40);
  lv_obj_align(btnBack, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_style_border_width(btnBack, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btnBack, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btnBack, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(btnBack, 0, LV_PART_MAIN);

  lv_obj_t *labelBack = lv_label_create(btnBack);
  lv_label_set_text(labelBack, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(labelBack, lv_color_hex(0x000000), 0);
  lv_obj_center(labelBack);

  lv_obj_add_event_cb(
    btnBack, [](lv_event_t *e) {
      lv_scr_load(mainScreen);
      stop_camera_stream_server();
      camera_streaming = false;
    },
    LV_EVENT_CLICKED, NULL);
}

void create_screens() {
  evScreen = lv_obj_create(NULL);
  homeScreen = lv_obj_create(NULL);
  meterScreen = lv_obj_create(NULL);
  btnAttendance = lv_obj_create(NULL);

  lv_obj_set_style_bg_color(evScreen, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
  lv_obj_set_style_bg_color(homeScreen, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
  lv_obj_set_style_bg_color(meterScreen, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
  lv_obj_set_style_bg_color(btnAttendance, lv_color_hex(0xDDDDDD), LV_PART_MAIN);

  add_back_button(evScreen);
  add_back_button(homeScreen);
  add_back_button(meterScreen);
  add_back_button(btnAttendance);

  // Tạo widget camera trên trang EV
  camera_img = create_camera_widget(evScreen);

  // Tạo màn hình vân tay
  create_fingerprint_screen();
}

void create_button_container() {
  btnContainer = lv_obj_create(mainScreen);
  lv_obj_set_size(btnContainer, 320, 100);
  lv_obj_align(btnContainer, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_border_width(btnContainer, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btnContainer, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btnContainer, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(btnContainer, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_scroll_dir(btnContainer, LV_DIR_HOR);
  lv_obj_set_style_radius(btnContainer, 10, LV_PART_MAIN);
  lv_obj_set_layout(btnContainer, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(btnContainer, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btnContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
}

void create_buttons() {
  // btnEV = lv_btn_create(btnContainer);
  // lv_obj_set_size(btnEV, 100, 60);

  // btnFinger = lv_btn_create(btnContainer);
  // lv_obj_set_size(btnFinger, 100, 60);

  btnAttendance = lv_btn_create(btnContainer);
  lv_obj_set_size(btnAttendance, 100, 60);
  
  btnRegister = lv_btn_create(btnContainer);
  lv_obj_set_size(btnRegister, 100, 60);

  btnWifi = lv_btn_create(btnContainer);
  lv_obj_set_size(btnWifi, 100, 60);

  // lv_obj_add_event_cb(btnEV, btn_event_cb, LV_EVENT_CLICKED, NULL);
  // lv_obj_add_event_cb(btnFinger, btn_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btnWifi, btn_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btnAttendance, btn_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(btnRegister, btn_event_cb, LV_EVENT_CLICKED, NULL);

  

  // lv_obj_t *labelEV = lv_label_create(btnEV);
  // lv_label_set_text(labelEV, "Camera");
  // lv_obj_center(labelEV);


  // lv_obj_t *labelFinger = lv_label_create(btnFinger);
  // lv_label_set_text(labelFinger, "Van tay");
  // lv_obj_center(labelFinger);

  lv_obj_t *labelAttendance = lv_label_create(btnAttendance);
  lv_label_set_text(labelAttendance, "Diem danh");
  lv_obj_center(labelAttendance);

  lv_obj_t *labelRegister = lv_label_create(btnRegister);
  lv_label_set_text(labelRegister, "Dang ky");
  lv_obj_center(labelRegister);

  lv_obj_t *labelHome = lv_label_create(btnWifi);
  lv_label_set_text(labelHome, "Wifi");
  lv_obj_center(labelHome);
}

void create_ui() {
  create_main_screen();
  create_labels();
  create_screens();
  create_button_container();
  create_buttons();
}

void page_anim_cb(void *var, int32_t v) {
  lv_obj_t *page = (lv_obj_t *)var;
  lv_obj_set_x(page, v);
}

void switch_page(lv_obj_t *current, lv_obj_t *next) {
  // Đảm bảo các tham số hợp lệ
  if (!current || !next) {
    Serial.println("Lỗi: Tham số không hợp lệ trong switch_page");
    return;
  }

  // Sử dụng animation đơn giản hơn để tránh vấn đề bộ nhớ
  lv_scr_load_anim(next, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);

  // Không gọi lv_refr_now() ngay lập tức để tránh xung đột với animation
  // Thay vào đó, để LVGL tự xử lý việc làm mới màn hình
}



void btn_event_cb(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);

  // Xóa timer cũ nếu tồn tại để tránh rò rỉ bộ nhớ
  if (page_switch_timer) {
    lv_timer_del(page_switch_timer);
    page_switch_timer = NULL;
  }

  if (btn == btnEV) {
    switch_page(mainScreen, evScreen);

    // Tạo một timer để bật streaming camera sau khi chuyển trang
    page_switch_timer = lv_timer_create([](lv_timer_t *timer) {
      camera_streaming = true;

      // Khởi động lại timer camera nếu đã tạm dừng
      extern lv_timer_t *camera_update_timer;
      if (camera_update_timer) {
        lv_timer_resume(camera_update_timer);
      }

      start_camera_stream_server();

      lv_timer_del(timer);
      page_switch_timer = NULL;
    },
                                        310, NULL);
  }  else if (btn == btnWifi) {
    camera_streaming = false;
    stop_camera_stream_server();

    // Tạo nếu chưa có
    if (wifiScreen == NULL) {
      create_wifi_screen();
    }

    // Chuyển trang an toàn
    switch_page(mainScreen, wifiScreen);

    // Tạo một timer để xóa mainScreen sau khi đã chuyển trang xong
    page_switch_timer = lv_timer_create([](lv_timer_t *t) {
        if (mainScreen) {
            lv_obj_del(mainScreen);
            mainScreen = NULL;
        }

        lv_timer_del(t);
        page_switch_timer = NULL;
    }, 400, NULL); // delay sau animation
}
 else if (btn == btnRegister) {
    camera_streaming = false;
    start_registration_flow();
  } else if (btn == btnThermo) {
    camera_streaming = false;  // Tắt streaming khi chuyển trang
    stop_camera_stream_server();
    switch_page(mainScreen, btnAttendance);
  } else if (btn == btnFinger) {
    camera_streaming = false;  // Tắt streaming khi chuyển trang
    stop_camera_stream_server();
    // Nếu màn hình vân tay đã tồn tại, xóa nó trước
    if (fingerprintScreen) {
      // Quan trọng: Hủy timer liên quan đến màn hình cũ nếu có
      if (fpTimer) {
        lv_timer_del(fpTimer);
        fpTimer = NULL;
      }
      
      // Xóa bàn phím số nếu đang hiển thị
      if (fpNumpad) {
        lv_obj_del(fpNumpad);
        fpNumpad = NULL;
      }
      
      if (fpNumpadTitle) {
        lv_obj_del(fpNumpadTitle);
        fpNumpadTitle = NULL;
      }

      lv_obj_del(fingerprintScreen);
      fingerprintScreen = NULL;
      
      // Reset các con trỏ UI khác liên quan đến màn hình này về NULL
      fpStatusLabel = NULL;
      fpActionBtn = NULL;
      fpDeleteBtn = NULL;
      fpEnrollBtn = NULL;
      fpVerifyBtn = NULL;
    }
    
    // Khởi động lại cảm biến vân tay
    if (!fingerprint.begin()) {
      Serial.println("ERROR: Không thể khởi động lại cảm biến vân tay!");
    }

    // Tạo lại UI vân tay
    create_fingerprint_screen(FP_MODE_FULL);

    // Chỉ chuyển trang nếu tạo màn hình thành công
    if (fingerprintScreen) {
      switch_page(mainScreen, fingerprintScreen);
    } else {
      Serial.println("LỖI: Không thể tạo màn hình vân tay!");
    }
  } else if (btn == btnAttendance){
    start_attendance_flow();
  }
}




