#ifndef ATTENDANCE_FLOW_H
#define ATTENDANCE_FLOW_H

#include <Arduino.h>

extern void start_attendance_flow();                         // Bắt đầu luồng điểm danh
extern void on_face_result_received(bool success, const String& mssv, const String& name, const String& className); // Web gọi hàm này sau khi nhận diện khuôn mặt
// // extern void fetch_template_and_verify(const String& mssv, const String& name);              // Lấy mẫu vân tay (không dùng nữa) (không dùng nữa)
extern void on_fingerprint_verified(bool matched, const String& mssv, const String& name);  // Kết quả xác thực vân tay
extern void record_attendance(const String& mssv);                                           // Gửi kết quả điểm danh
extern void show_result_screen(bool success, const String& mssv, const String& name, const String& className);// Hiển thị kết quả
extern void on_attendance_success_received(const String& name);                             // Xử lý khi web điểm danh thành công

#endif
