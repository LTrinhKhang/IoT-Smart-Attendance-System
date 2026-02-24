#ifndef FINGERPRINT_UI_H
#define FINGERPRINT_UI_H

#include <lvgl.h>
#include "Finger.h"  // thư viện giao tiếp vân tay
#include <functional>


// Biến cho màn hình vân tay
extern lv_obj_t *fingerprintScreen;
extern lv_obj_t *fpStatusLabel;
extern lv_obj_t *fpActionBtn;
extern lv_obj_t *fpDeleteBtn;
extern lv_obj_t *fpEnrollBtn;
extern lv_obj_t *fpVerifyBtn ;
extern lv_obj_t *fpNumpad;
extern lv_obj_t *fpNumpadTitle ;




// Trạng thái giao diện vân tay
enum FingerprintUIState {
    FP_STATE_IDLE,
    FP_STATE_ENROLLING,
    FP_STATE_VERIFYING,
    FP_STATE_DELETING
};

enum FingerprintScreenMode {
    FP_MODE_FULL,     // Đầy đủ Enroll/Delete/Verify
    FP_MODE_VERIFY,    // Chỉ Verify
    FP_MODE_ENROLL_ONLY
};

// Biến truy cập bên ngoài
extern lv_obj_t *fingerprintScreen;
extern lv_timer_t *fpTimer;
extern bool shouldDeleteFpTimer;
extern int selectedID;


// Giao diện và xử lý chính
void create_fingerprint_screen(FingerprintScreenMode mode = FP_MODE_FULL);
void fingerprint_btn_event_handler(lv_event_t *e);
void update_fingerprint_status(int status, const char* message);
void start_fingerprint_enrollment();
typedef std::function<void(bool)> VerificationCallback;
void start_fingerprint_verification(VerificationCallback cb);
void start_fingerprint_template_verification(VerificationCallback cb);
void start_fingerprint_mssv_verification(uint32_t mssv, VerificationCallback cb);
void start_fingerprint_deletion();
void show_fingerprint_numpad(const char* title, lv_event_cb_t callback);
void enrollment_callback(int status, const char *message);



#endif
