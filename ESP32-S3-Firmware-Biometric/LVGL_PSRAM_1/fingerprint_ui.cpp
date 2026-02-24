// === Chức năng vân tay ===
#include "fingerprint_ui.h"
#include "lvgl_ui.h"
#include "http_client.h"
#include "registration_ui.h"
#include "globals.h"





// Định nghĩa hằng số
#define FP_TIMEOUT_MS 10000  // Timeout cho xác thực vân tay (10 giây)

// Biến cho màn hình vân tay
lv_obj_t *fingerprintScreen = NULL;
lv_obj_t *fpStatusLabel = NULL;
lv_obj_t *fpActionBtn = NULL;
lv_obj_t *fpDeleteBtn = NULL;
lv_obj_t *fpEnrollBtn = NULL;
lv_obj_t *fpVerifyBtn = NULL;
lv_obj_t *fpNumpad = NULL;
lv_obj_t *fpNumpadTitle = NULL;

static FingerprintUIState currentState = FP_STATE_IDLE;
// Biến trạng thái vân tay
bool shouldDeleteFpTimer = false;
int selectedID = 0;
lv_timer_t *fpTimer = NULL;


// Callback cho bàn phím số
static lv_event_cb_t numpadCallback = NULL;

static VerificationCallback verificationCallback = nullptr;




// Hàm tiện ích để reset UI vân tay về trạng thái ban đầu
static void reset_fingerprint_ui(const char *message = "Sẵn sàng") {
  // Đặt trạng thái
  currentState = FP_STATE_IDLE;
  
  // Cập nhật trạng thái hiển thị
  if (fpStatusLabel) {
    lv_label_set_text(fpStatusLabel, message);
    lv_obj_set_style_text_color(fpStatusLabel, lv_color_hex(0x000000), LV_PART_MAIN);
  }

  // Hiện nút nào tồn tại
  if (fpEnrollBtn)  lv_obj_clear_flag(fpEnrollBtn, LV_OBJ_FLAG_HIDDEN);
  if (fpVerifyBtn)  lv_obj_clear_flag(fpVerifyBtn, LV_OBJ_FLAG_HIDDEN);
  if (fpDeleteBtn)  lv_obj_clear_flag(fpDeleteBtn, LV_OBJ_FLAG_HIDDEN);

  // Ẩn nút Cancel nếu có
  if (fpActionBtn)  lv_obj_add_flag(fpActionBtn, LV_OBJ_FLAG_HIDDEN);
}

// Hàm tạo màn hình vân tay
void create_fingerprint_screen(FingerprintScreenMode mode) {
  // Tạo màn hình
  fingerprintScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(fingerprintScreen, lv_color_hex(0xDDDDDD), LV_PART_MAIN);

  // Thêm nút quay lại
  add_back_button(fingerprintScreen);

  // Tạo tiêu đề
  lv_obj_t *title = lv_label_create(fingerprintScreen);
  lv_label_set_text(title, "Van Tay");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

  // Tạo nhãn trạng thái
  fpStatusLabel = lv_label_create(fingerprintScreen);
  lv_label_set_text(fpStatusLabel, "San Sang");
  lv_obj_set_width(fpStatusLabel, 280);
  lv_label_set_long_mode(fpStatusLabel, LV_LABEL_LONG_WRAP);
  lv_obj_align(fpStatusLabel, LV_ALIGN_TOP_MID, 0, 50);

  // Tạo container cho các nút
  lv_obj_t *btnContainer = lv_obj_create(fingerprintScreen);
  lv_obj_set_size(btnContainer, 280, 120);
  lv_obj_align(btnContainer, LV_ALIGN_CENTER, 0, 20);
  lv_obj_set_style_bg_opa(btnContainer, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btnContainer, 0, LV_PART_MAIN);
  lv_obj_set_flex_flow(btnContainer, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(btnContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  
  if (mode == FP_MODE_FULL) {
    // Tạo các nút chức năng
    fpVerifyBtn = lv_btn_create(btnContainer);
    lv_obj_set_size(fpVerifyBtn, 130, 50);
    lv_obj_add_event_cb(fpVerifyBtn, fingerprint_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_t *verifyLabel = lv_label_create(fpVerifyBtn);
    lv_label_set_text(verifyLabel, "Verify");
    lv_obj_center(verifyLabel);

    // Chỉ thêm Enroll và Delete nếu ở chế độ đầy đủ
    fpEnrollBtn = lv_btn_create(btnContainer);
    lv_obj_set_size(fpEnrollBtn, 130, 50);
    lv_obj_add_event_cb(fpEnrollBtn, fingerprint_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_t *enrollLabel = lv_label_create(fpEnrollBtn);
    lv_label_set_text(enrollLabel, "Enroll");
    lv_obj_center(enrollLabel);

    fpDeleteBtn = lv_btn_create(btnContainer);
    lv_obj_set_size(fpDeleteBtn, 130, 50);
    lv_obj_add_event_cb(fpDeleteBtn, fingerprint_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_t *deleteLabel = lv_label_create(fpDeleteBtn);
    lv_label_set_text(deleteLabel, "Delete");
    lv_obj_center(deleteLabel);
  } else {
    fpEnrollBtn = NULL;
    fpDeleteBtn = NULL;
    fpVerifyBtn = NULL;
  }
  // Nút hành động (ẩn ban đầu)
  fpActionBtn = lv_btn_create(fingerprintScreen);
  lv_obj_set_size(fpActionBtn, 200, 50);
  lv_obj_align(fpActionBtn, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_add_event_cb(fpActionBtn, fingerprint_btn_event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_t *actionLabel = lv_label_create(fpActionBtn);
  lv_label_set_text(actionLabel, "Cancel");
  lv_obj_center(actionLabel);
  lv_obj_add_flag(fpActionBtn, LV_OBJ_FLAG_HIDDEN);
}

// Cập nhật trạng thái hiển thị
void update_fingerprint_status(int status, const char *message) {
  if (!fpStatusLabel) return;

  lv_label_set_text(fpStatusLabel, message);

  // Đổi màu dựa vào trạng thái
  if (status < 0) {
    // Lỗi - màu đỏ
    lv_obj_set_style_text_color(fpStatusLabel, lv_color_hex(0xFF0000), LV_PART_MAIN);
  } else if (status > 0) {
    // Thành công - màu xanh lá
    lv_obj_set_style_text_color(fpStatusLabel, lv_color_hex(0x00AA00), LV_PART_MAIN);
  } else {
    // Thông tin - màu đen
    lv_obj_set_style_text_color(fpStatusLabel, lv_color_hex(0x000000), LV_PART_MAIN);
  }
}

// Callback cho bàn phím số
static void numpad_event_cb(lv_event_t *e) {
  if (e == NULL) {
    Serial.println("ERROR: Null event in numpad_event_cb");
    return;
  }

  if (fpNumpad == NULL || fpNumpadTitle == NULL) {
    Serial.println("ERROR: fpNumpad or fpNumpadTitle is NULL in numpad_event_cb");
    return;
  }

  lv_obj_t *btn = lv_event_get_target(e);
  if (btn == NULL) {
    Serial.println("ERROR: Null button in numpad_event_cb");
    return;
  }

  uint32_t btn_id = lv_btnmatrix_get_selected_btn(fpNumpad);
  const char *txt = lv_btnmatrix_get_btn_text(fpNumpad, btn_id);
  if (txt == NULL) {
    Serial.println("ERROR: Null button text in numpad_event_cb");
    return;
  }

  // ========== XỬ LÝ NÚT ENTER ==========
  if (strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) {
    const char *raw_input = lv_textarea_get_text(fpNumpadTitle);
    Serial.printf("DEBUG: Raw input string = [%s], length = %d\n", raw_input, strlen(raw_input));

    // Copy về buffer an toàn
    char value_str[16];
    strncpy(value_str, raw_input, sizeof(value_str) - 1);
    value_str[sizeof(value_str) - 1] = '\0';  // Đảm bảo null-terminated

    // Debug ký tự từng byte
    for (size_t i = 0; i < strlen(value_str); i++) {
      Serial.printf("char %d: %c (0x%02x)\n", i, value_str[i], value_str[i]);
    }

    if (strlen(value_str) == 0) {
      Serial.println("WARNING: Empty input in numpad");
      return;
    }

    // Chuyển đổi an toàn
    char *endptr;
    errno = 0;
    long convertedValue = strtol(value_str, &endptr, 10);

    if (*endptr != '\0') {
      Serial.printf("ERROR: Invalid character at position %d\n", (int)(endptr - value_str));
      return;
    }
    if (errno == ERANGE) {
      Serial.println("ERROR: Number out of range");
      return;
    }
    if (convertedValue < 0) {
      Serial.println("ERROR: Negative value not allowed");
      return;
    }
    if (convertedValue > INT_MAX) {
      Serial.println("ERROR: Number too large");
      return;
    }

    selectedID = (int)convertedValue;
    Serial.printf("DEBUG: selectedID = %d\n", selectedID);

    // Xóa bàn phím
    lv_event_cb_t saved_callback = numpadCallback;
    lv_obj_del(fpNumpad);       fpNumpad = NULL;
    lv_obj_del(fpNumpadTitle);  fpNumpadTitle = NULL;

    // Gọi callback
    if (saved_callback) {
      lv_event_t event;
      saved_callback(&event);
    } else {
      Serial.println("ERROR: Null callback in numpad_event_cb");
    }
  }

  // ========== XỬ LÝ NÚT HỦY ==========
  else if (strcmp(txt, "HUY") == 0 || strcmp(txt, "Hủy") == 0) {
    lv_obj_del(fpNumpad);       fpNumpad = NULL;
    lv_obj_del(fpNumpadTitle);  fpNumpadTitle = NULL;
    reset_fingerprint_ui("Đã hủy thao tác");
  }

  // ========== XỬ LÝ SỐ ==========
  else if (strlen(txt) == 1 && isdigit(txt[0])) {
    lv_textarea_add_char(fpNumpadTitle, txt[0]);
  }

  // ========== XỬ LÝ BACKSPACE ==========
  else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
    const char *current_text = lv_textarea_get_text(fpNumpadTitle);
    if (current_text && strlen(current_text) > 0) {
      lv_textarea_del_char(fpNumpadTitle);
    }
  }
}


// Hiển thị bàn phím số
void show_fingerprint_numpad(const char *title, lv_event_cb_t callback) {
  // Kiểm tra tham số
  if (title == NULL || callback == NULL) {
    Serial.println("ERROR: Invalid parameters in show_fingerprint_numpad");
    return;
  }

  // Kiểm tra màn hình vân tay
  if (fingerprintScreen == NULL) {
    Serial.println("ERROR: fingerprintScreen is NULL in show_fingerprint_numpad");
    return;
  }

  // Xóa bàn phím cũ nếu tồn tại
  if (fpNumpad != NULL) {
    lv_obj_del(fpNumpad);
    fpNumpad = NULL;
  }

  // Xóa ô nhập liệu cũ nếu tồn tại
  if (fpNumpadTitle != NULL) {
    lv_obj_del(fpNumpadTitle);
    fpNumpadTitle = NULL;
  }

  // Định nghĩa bản đồ nút
  static const char *btnm_map[] = { "1", "2", "3", "\n",
                                    "4", "5", "6", "\n",
                                    "7", "8", "9", "\n",
                                    LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_NEW_LINE, "\n",
                                    "HUY", "" };

  // Lưu callback
  numpadCallback = callback;

  // Tạo ô nhập liệu
  fpNumpadTitle = lv_textarea_create(fingerprintScreen);
  if (fpNumpadTitle == NULL) {
    Serial.println("ERROR: Failed to create fpNumpadTitle");
    return;
  }

  lv_textarea_set_text(fpNumpadTitle, "");
  lv_textarea_set_placeholder_text(fpNumpadTitle, title);
  lv_textarea_set_one_line(fpNumpadTitle, true);
  lv_obj_set_width(fpNumpadTitle, 200);
  lv_obj_align(fpNumpadTitle, LV_ALIGN_TOP_MID, 0, 100);
  lv_textarea_set_max_length(fpNumpadTitle, 8);  // Tăng lên 8 để hỗ trợ MSSV 8 chữ số
  lv_textarea_set_accepted_chars(fpNumpadTitle, "0123456789");

  // Tạo bàn phím
  fpNumpad = lv_btnmatrix_create(fingerprintScreen);
  if (fpNumpad == NULL) {
    Serial.println("ERROR: Failed to create fpNumpad");
    // Xóa ô nhập liệu nếu không tạo được bàn phím
    lv_obj_del(fpNumpadTitle);
    fpNumpadTitle = NULL;
    return;
  }

  lv_btnmatrix_set_map(fpNumpad, btnm_map);
  lv_obj_set_size(fpNumpad, 200, 200);
  lv_obj_align_to(fpNumpad, fpNumpadTitle, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
  lv_obj_add_event_cb(fpNumpad, numpad_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // Reset selectedID
  selectedID = 0;
}

void fingerprint_btn_event_handler(lv_event_t *e) {
  if (e == NULL) {
    Serial.println("ERROR: Null event in fingerprint_btn_event_handler");
    return;
  }

  lv_obj_t *btn = lv_event_get_target(e);
  if (btn == NULL) {
    Serial.println("ERROR: Null button in fingerprint_btn_event_handler");
    return;
  }

  uint32_t freeHeapBefore = ESP.getFreeHeap();

  if (btn == fpEnrollBtn && fpEnrollBtn != NULL) {
    Serial.println("Starting fingerprint enrollment...");
    start_fingerprint_enrollment();
  } else if (btn == fpVerifyBtn && fpVerifyBtn != NULL) {
    Serial.println("Starting fingerprint verification...");
    start_fingerprint_verification([](bool matched) {
        if (matched) {
            update_fingerprint_status(0, "Xac thuc thanh cong!");
        } else {
            update_fingerprint_status(-1, "Van tay khong khop");
        }
    });
  } else if (btn == fpDeleteBtn && fpDeleteBtn != NULL) {
    Serial.println("Starting fingerprint deletion...");
    start_fingerprint_deletion();
  } else if (btn == fpActionBtn && fpActionBtn != NULL) {
    Serial.println("Canceling current fingerprint operation...");

    if (fpTimer) {
      lv_timer_del(fpTimer);
      fpTimer = NULL;
    }

    if (fpNumpad) {
      lv_obj_del(fpNumpad);
      fpNumpad = NULL;
    }

    if (fpNumpadTitle) {
      lv_obj_del(fpNumpadTitle);
      fpNumpadTitle = NULL;
    }

    reset_fingerprint_ui("Đã hủy thao tác");
  } else {
    Serial.println("WARNING: Unknown or NULL button pressed in fingerprint_btn_event_handler");
  }

  uint32_t freeHeapAfter = ESP.getFreeHeap();
  if (freeHeapBefore - freeHeapAfter > 1000) {
    Serial.printf("WARNING: Large heap usage in button handler: %d bytes\n",
                  freeHeapBefore - freeHeapAfter);
  }
}


// Callback cho quá trình đăng ký
void enrollment_callback(int status, const char *message) {
  update_fingerprint_status(status, message);

  // Kiểm tra nếu đăng ký thành công (status > 0 là sensor ID, hoặc message chứa "thành công")
  if (status > 0 || (status == 0 && strstr(message, "thành công") != NULL)) {
    Serial.printf("✅ Đăng ký vân tay thành công! Sensor ID: %d\n", status);
    
    // ✅ Đợi 2s rồi reset UI và quay về màn hình chính
    if (fpTimer) {
      lv_timer_del(fpTimer);
      fpTimer = NULL;
    }
    
    fpTimer = lv_timer_create([](lv_timer_t *timer) {
      reset_fingerprint_ui();
      
      // Hiển thị thông báo thành công
      update_fingerprint_status(0, "Hoan tat dang ky!");
      
      // Xóa timer hiện tại trước khi tạo timer mới
      if (fpTimer) {
        lv_timer_del(fpTimer);
        fpTimer = NULL;
      }
      
      // Tạo timer để quay về màn hình chính
      fpTimer = lv_timer_create([](lv_timer_t *t) {
        lv_scr_load(mainScreen);
        if (fpTimer) {
          lv_timer_del(fpTimer);
          fpTimer = NULL;
        }
      }, 2000, NULL);
      
    }, 2000, NULL);

  } else if (status < 0) {
    // Trường hợp lỗi
    Serial.printf("❌ Đăng ký thất bại với mã lỗi: %d\n", status);
    
    // Hiển thị nút thử lại sau 2 giây
    if (fpTimer) {
      lv_timer_del(fpTimer);
      fpTimer = NULL;
    }
    
    fpTimer = lv_timer_create([](lv_timer_t *timer) {
      // Hiển thị nút thử lại
      if (fpActionBtn) {
        lv_obj_t* label = lv_obj_get_child(fpActionBtn, 0);
        if (label) lv_label_set_text(label, "Thử lại");
        lv_obj_clear_flag(fpActionBtn, LV_OBJ_FLAG_HIDDEN);
      }
      
      if (fpTimer) {
        lv_timer_del(fpTimer);
        fpTimer = NULL;
      }
    }, 2000, NULL);
  }
}



// Callback cho quá trình xác thực
static void verification_callback(int status, const char *message) {
  // Kiểm tra tham số
  if (message == NULL) {
    Serial.println("ERROR: Null message in verification_callback");
    update_fingerprint_status(-1, "Lỗi: Thông báo không hợp lệ");
    return;
  }

  // Cập nhật trạng thái
  update_fingerprint_status(status, message);

  // Xóa timer cũ nếu tồn tại
  if (fpTimer) {
    lv_timer_del(fpTimer);
    fpTimer = NULL;
  }

  // Nếu xác thực thành công hoặc có lỗi, tạo timer để trở về trạng thái ban đầu
  if (status == 0 || status != 0) {
    // Tạo timer mới để trở về trạng thái ban đầu sau 2-3 giây
    int delay = (status == 0) ? 3000 : 2000;
    
    fpTimer = lv_timer_create([](lv_timer_t *timer) {
      reset_fingerprint_ui();

      // Xóa timer
      lv_timer_del(timer);
      fpTimer = NULL;
    }, delay, NULL);
  }
}

// Callback cho quá trình xóa
static void deletion_callback(int status, const char *message) {
  // Kiểm tra tham số
  if (message == NULL) {
    Serial.println("ERROR: Null message in deletion_callback");
    update_fingerprint_status(-1, "Lỗi: Thông báo không hợp lệ");
    return;
  }

  // Cập nhật trạng thái
  update_fingerprint_status(status, message);

  // Xóa timer cũ nếu tồn tại
  if (fpTimer) {
    lv_timer_del(fpTimer);
    fpTimer = NULL;
  }

  // Nếu xóa thành công hoặc có lỗi, tạo timer để trở về trạng thái ban đầu
  if (status == 0 || status != 0) {
    // Tạo timer mới để trở về trạng thái ban đầu sau 2 giây
    fpTimer = lv_timer_create([](lv_timer_t *timer) {
      reset_fingerprint_ui();

      // Xóa timer
      lv_timer_del(timer);
      fpTimer = NULL;
    }, 2000, NULL);
  }
}

// Bắt đầu quá trình đăng ký vân tay
void start_fingerprint_enrollment() {
  currentState = FP_STATE_ENROLLING;

  // Ẩn các nút chức năng
  lv_obj_add_flag(fpEnrollBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(fpVerifyBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(fpDeleteBtn, LV_OBJ_FLAG_HIDDEN);

  // Hiện nút hủy
  lv_obj_clear_flag(fpActionBtn, LV_OBJ_FLAG_HIDDEN);

  // Hiển thị bàn phím để nhập MSSV (hoặc ID nếu bạn dùng ID riêng)
  show_fingerprint_numpad("Nhập MSSV", [](lv_event_t *e) {
    if (selectedID < 10000000 || selectedID > 99999999) {
      update_fingerprint_status(-1, "MSSV không hợp lệ");

      if (fpTimer) lv_timer_del(fpTimer);
      fpTimer = lv_timer_create([](lv_timer_t *timer) {
        reset_fingerprint_ui();
        lv_timer_del(timer);
        fpTimer = NULL;
      }, 2000, NULL);
      return;
    }

    update_fingerprint_status(0, "🟡 Bat dau đăng ký vân tay...");

    if (fpTimer) lv_timer_del(fpTimer);
    fpTimer = lv_timer_create([](lv_timer_t *timer) {
      fingerprint.enrollFingerprint(selectedID, enrollment_callback);  // luôn ID = 1
      lv_timer_del(timer);
      fpTimer = NULL;
    }, 500, NULL);
  });
}


// Bắt đầu quá trình xác thực vân tay
void start_fingerprint_verification(VerificationCallback cb) {
  currentState = FP_STATE_VERIFYING;
  verificationCallback = cb;

  if (!fpActionBtn) {
    Serial.println("ERROR: fpActionBtn is NULL in start_fingerprint_verification");
    return;
  }

  if (fpEnrollBtn) lv_obj_add_flag(fpEnrollBtn, LV_OBJ_FLAG_HIDDEN);
  if (fpVerifyBtn) lv_obj_add_flag(fpVerifyBtn, LV_OBJ_FLAG_HIDDEN);
  if (fpDeleteBtn) lv_obj_add_flag(fpDeleteBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(fpActionBtn, LV_OBJ_FLAG_HIDDEN);

  update_fingerprint_status(0, "Dat van tay de xac thực...");

  if (fpTimer) {
    lv_timer_del(fpTimer);
    fpTimer = NULL;
  }

  fpTimer = lv_timer_create([](lv_timer_t *timer) {
    int16_t result = fingerprint.checkFingerprint(FP_TIMEOUT_MS, [](int status, const char* message) {
      update_fingerprint_status(status, message);
      if (verificationCallback) {
        verificationCallback(status == 0);
      }
    });

    if (result < 0) {
      if (fpTimer) {
        lv_timer_del(fpTimer);
        fpTimer = NULL;
      }

      fpTimer = lv_timer_create([](lv_timer_t *t2) {
        reset_fingerprint_ui();
        shouldDeleteFpTimer = true;
      }, 2000, NULL);
    }

    shouldDeleteFpTimer = true;
  }, 500, NULL);
}

// Bắt đầu quá trình xác thực vân tay với MSSV cụ thể
void start_fingerprint_mssv_verification(uint32_t mssv, VerificationCallback cb) {
    currentState = FP_STATE_VERIFYING;
    verificationCallback = cb;

    if (!fpActionBtn) {
        Serial.println("ERROR: fpActionBtn is NULL");
        return;
    }

    if (fpEnrollBtn) lv_obj_add_flag(fpEnrollBtn, LV_OBJ_FLAG_HIDDEN);
    if (fpVerifyBtn) lv_obj_add_flag(fpVerifyBtn, LV_OBJ_FLAG_HIDDEN);
    if (fpDeleteBtn) lv_obj_add_flag(fpDeleteBtn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(fpActionBtn, LV_OBJ_FLAG_HIDDEN);

    update_fingerprint_status(0, "Đặt tay lên cảm biến để xác thực...");

    if (fpTimer) {
        lv_timer_del(fpTimer);
        fpTimer = nullptr;
    }

    // 🛠 Dùng biến global tạm giữ để truyền MSSV đúng vào timer
    static uint32_t mssvHolder = 0;
    mssvHolder = mssv;

    fpTimer = lv_timer_create([](lv_timer_t *timer) {
        uint32_t* mssvPtr = (uint32_t*)timer->user_data;
        if (!mssvPtr || *mssvPtr == 0) {
            Serial.println("❌ MSSV không hợp lệ trong timer");
            if (verificationCallback) verificationCallback(false);
            return;
        }

        uint32_t mssv = *mssvPtr;
        Serial.printf("🔍 Đang xác thực MSSV %u...\n", mssv);

        int resultCode = fingerprint.verifyAgainstMSSV(mssv, FP_TIMEOUT_MS, [](int status, const char* msg) {
            update_fingerprint_status(status, msg);
        });

        if (verificationCallback) {
            bool match = (resultCode == FP_ERROR_NONE);
            Serial.printf("📌 Gọi callback xác thực: matched = %s\n", match ? "true" : "false");
            verificationCallback(match);
        }
        stop
        if (fpTimer) {
            lv_timer_del(fpTimer);
            fpTimer = nullptr;
        }
        shouldDeleteFpTimer = true;
    }, 500, &mssvHolder);
}




// Bắt đầu quá trình xóa vân tay

void start_fingerprint_deletion() {
  currentState = FP_STATE_DELETING;

  if (!fpActionBtn) {
    Serial.println("ERROR: fpActionBtn is NULL in start_fingerprint_verification");
    return;
  }

  // Nếu không dùng các nút khác, không cần kiểm tra nữa
  if (fpEnrollBtn) lv_obj_add_flag(fpEnrollBtn, LV_OBJ_FLAG_HIDDEN);
  if (fpVerifyBtn) lv_obj_add_flag(fpVerifyBtn, LV_OBJ_FLAG_HIDDEN);
  if (fpDeleteBtn) lv_obj_add_flag(fpDeleteBtn, LV_OBJ_FLAG_HIDDEN);

lv_obj_clear_flag(fpActionBtn, LV_OBJ_FLAG_HIDDEN);

  lv_obj_clear_flag(fpActionBtn, LV_OBJ_FLAG_HIDDEN);

  show_fingerprint_numpad("Nhập ID cần xóa", [](lv_event_t *e) {
    if (selectedID < 1 || selectedID > 127) {
      update_fingerprint_status(-1, "ID không hợp lệ. Phải từ 1-127.");

      if (fpTimer) {
        lv_timer_del(fpTimer);
        fpTimer = NULL;
      }

      fpTimer = lv_timer_create([](lv_timer_t *timer) {
        reset_fingerprint_ui();
        shouldDeleteFpTimer = true;
      }, 2000, NULL);
      return;
    }

    update_fingerprint_status(0, "Đang xóa vân tay...");

    if (fpTimer) {
      lv_timer_del(fpTimer);
      fpTimer = NULL;
    }

    fpTimer = lv_timer_create([](lv_timer_t *timer) {
      uint32_t freeHeap = ESP.getFreeHeap();
      Serial.printf("Free heap before deletion: %d bytes\n", freeHeap);

      uint8_t id_to_delete = selectedID;
      Serial.printf("Deleting fingerprint ID: %d\n", id_to_delete);

      int8_t result = fingerprint.deleteFingerprint(id_to_delete, deletion_callback);

      if (result != FP_ERROR_NONE) {
        Serial.printf("Deletion failed with code: %d\n", result);
        update_fingerprint_status(-1, "Xóa vân tay thất bại");

        // Tạo timer về lại giao diện sau 2s
        if (fpTimer) {
          lv_timer_del(fpTimer);
          fpTimer = NULL;
        }

        fpTimer = lv_timer_create([](lv_timer_t *t2) {
          reset_fingerprint_ui();
          shouldDeleteFpTimer = true;
        }, 2000, NULL);
      }

      freeHeap = ESP.getFreeHeap();
      Serial.printf("Free heap after deletion: %d bytes\n", freeHeap);

      shouldDeleteFpTimer = true;

    }, 500, NULL);
  });
}



