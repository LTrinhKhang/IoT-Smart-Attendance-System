#ifndef LVGL_UI_H
#define LVGL_UI_H

#include <lvgl.h>
#include "Finger.h"

// Các biến giao diện đã có
extern lv_obj_t *clockLabel;
extern lv_obj_t *dateLabel;
extern lv_obj_t *weatherLabel;
extern lv_obj_t *btnEV;
extern lv_obj_t *btnHome;
extern lv_obj_t *btnRegister;
extern lv_obj_t *btnThermo;
extern lv_obj_t *btnFinger;  // Thêm nút vân tay
extern lv_obj_t *btnContainer;
extern lv_obj_t *mainScreen;
extern lv_obj_t *evScreen;
extern lv_obj_t *homeScreen;
extern lv_obj_t *meterScreen;
extern lv_obj_t *btnAttendance;

// Biến cho camera stream
extern lv_obj_t *camera_img;
extern bool camera_streaming;

// extern lv_timer_t *fpTimer;
// extern bool shouldDeleteFpTimer;




// // Trạng thái của module vân tay
// enum FingerprintUIState {
//     FP_STATE_IDLE,
//     FP_STATE_ENROLLING,
//     FP_STATE_VERIFYING,
//     FP_STATE_DELETING
// };

/**
 * @brief Tạo giao diện người dùng
 */
void create_ui();

/**
 * @brief Tạo màn hình chính
 */
void create_main_screen();

/**
 * @brief Tạo các nhãn hiển thị
 */
void create_labels();

/**
 * @brief Thêm nút quay lại vào màn hình
 * 
 * @param screen Màn hình cần thêm nút
 */
void add_back_button(lv_obj_t *screen);

/**
 * @brief Tạo các màn hình phụ
 */
void create_screens();

/**
 * @brief Tạo container chứa các nút
 */
void create_button_container();

/**
 * @brief Tạo các nút chức năng
 */
void create_buttons();

/**
 * @brief Chuyển đổi giữa các trang
 * 
 * @param current Trang hiện tại
 * @param next Trang tiếp theo
 */
void switch_page(lv_obj_t *current, lv_obj_t *next);

/**
 * @brief Callback cho hiệu ứng chuyển trang
 * 
 * @param var Biến đối tượng
 * @param v Giá trị
 */
void page_anim_cb(void *var, int32_t v);

/**
 * @brief Xử lý sự kiện nút
 * 
 * @param e Sự kiện
 */
void btn_event_cb(lv_event_t *e);

/**
 * @brief Xử lý phần điểm danh
 * 
 * @param e Sự kiện
 */
void start_attendance_flow();
void show_result_screen(bool success, const char *message);
#endif
