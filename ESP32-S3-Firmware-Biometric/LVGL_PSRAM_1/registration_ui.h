#ifndef REGISTRATION_UI_H
#define REGISTRATION_UI_H

void create_registration_screen_ui(uint32_t mssv);
void update_registration_ui(const char* progress, const char* status, bool showRegisterBtn);
void start_registration_flow();
void start_face_registration(uint32_t mssv);
void send_face_to_server(const String& base64, uint32_t mssv);
String capture_camera_image_base64();
void check_student_status(uint32_t mssv);
void start_fingerprint_enrollment_direct();
extern uint32_t currentMSSV;
#endif
