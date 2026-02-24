#ifndef _WIFISCREEN_H_
#define _WIFISCREEN_H_

#include <lvgl.h>


extern lv_obj_t *wifiScreen;          ///< Màn hình chính giao diện WiFi
extern lv_obj_t *wifiStatusLabel;     ///< Nhãn hiển thị trạng thái WiFi (đang kết nối, đã kết nối,...)
extern lv_obj_t *wifiList;            ///< Danh sách các mạng WiFi
extern lv_obj_t *wifiConnectBtn;      ///< Nút kết nối WiFi
extern lv_obj_t *wifiDisconnectBtn;   ///< Nút ngắt kết nối WiFi
extern lv_obj_t *wifiPasswordInput;   ///< Trường nhập mật khẩu WiFi
extern lv_obj_t *keyboard;            ///< Bàn phím ảo cho trường nhập mật khẩu
extern lv_obj_t *wifiReloadBtn;       ///< Nút quét lại mạng WiFi
extern bool wifiFlag;

/**
 * @brief Tạo giao diện WiFi trên màn hình
 */
void create_wifi_screen();

/**
 * @brief Cập nhật trạng thái WiFi
 *
 * @param status true nếu đã kết nối, false nếu đã ngắt
 * @param ssid   Tên mạng đã kết nối hoặc đang cố kết nối
 */
void event_handler(bool status, const char* ssid);

/**
 * @brief Kiểm tra trạng thái kết nối WiFi
 */
void check_wifi_status_cb();

/**
 * @brief Kiểm tra event của bàn phím
 */
void keyboard_event_cb(lv_event_t * e);

/**
 * @brief Event tạo list wifi
 */
void wifi_list_event_cb(lv_event_t *e);

/**
 * @brief Cleanup WiFi screen resources
 */
void cleanup_wifi_screen();

#endif // _WIFISCREEN_H_
