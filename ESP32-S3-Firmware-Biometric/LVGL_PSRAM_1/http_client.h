#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

// Biến cấu hình server
extern String serverIP;
extern int serverPort;
extern String serverURL;
extern bool isStreamActive;
extern WebServer streamServer;

// Hàm khởi tạo và cấu hình
void initHTTPClient();
void updateServerConfig(const String& ip, int port);
void printConnectionInfo();
void start_camera_stream_server();
void stop_camera_stream_server();
void camera_stream_server_handle();
void camera_stream_handler();
void camera_capture_handler();
void stream_task(void *param);
void send_template_to_server(int id, const String &base64Data);
void send_register_signal_to_web(uint32_t mssv);
void handle_fingerprint_register_request();
void send_attendance_signal_to_web();
String getCurrentTimestamp();
String get_template_by_mssv(const String& mssv);

#endif
