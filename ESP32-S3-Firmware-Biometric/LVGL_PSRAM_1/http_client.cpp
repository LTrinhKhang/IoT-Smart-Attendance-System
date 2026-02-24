#include "http_client.h"
#include <WiFi.h>
#include "esp_camera.h"
#include <ArduinoJson.h>
#include "registration_ui.h"
#include "fingerprint_ui.h"
#include "attendance_flow.h"



// Dảm bảo ban có dòng này Dể lay các biến/hàm từ file khác
extern lv_obj_t* regScreen;
extern uint32_t currentMSSV;

extern String studentID;
extern String studentName;
String studentClass = "";
extern bool faceRecognized;

extern void switch_page(lv_obj_t* from, lv_obj_t* to);
extern void create_fingerprint_screen(int mode);
extern void update_fingerprint_status(int code, const char* msg);
extern void start_fingerprint_enrollment();
extern void check_student_status(uint32_t mssv);


// Biến toàn cục
String serverIP = "192.168.0.4";  // IP mặc Dịnh
int serverPort = 5500;
String serverURL = "";            // URL Dầy Dủ

bool isStreamActive = false;  // Biến trang thái stream Dang bật hay tat


HTTPClient http;  // Instance HTTPClient

WebServer streamServer(81);

// Cập nhật cau hình server và tao URL
void updateServerConfig(const String& ip, int port) {
    serverIP = ip;
    serverPort = port;
    serverURL = "http://" + serverIP + ":" + String(serverPort);
}

// Khởi tao HTTP client
void initHTTPClient() {
    updateServerConfig(serverIP, serverPort);
    Serial.println("🌐 HTTP Client đã được khởi tạo");
}

// In thông tin kết nối
void printConnectionInfo() {
    Serial.println("🔍 ===== THÔNG TIN KẾT NỐI =====");
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("📶 WiFi: ✅ Đã kết nối");
        Serial.println("📡 SSID: " + WiFi.SSID());
        Serial.println("🔢 IP ESP32: " + WiFi.localIP().toString());
        Serial.println("🌍 URL đầy đủ: " + serverURL);
        Serial.println("📊 RSSI: " + String(WiFi.RSSI()) + " dBm");
    } else {
        Serial.println("📶 WiFi: ❌ Chưa kết nối");
    }
    Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("💾 Free PSRAM: " + String(ESP.getFreePsram()) + " bytes");
    
    Serial.println("================================");
}


void start_camera_stream_server() {

    // ==== OPTIONS /register-success ====
    streamServer.on("/register-success", HTTP_OPTIONS, []() {
        streamServer.sendHeader("Access-Control-Allow-Origin", "*");
        streamServer.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        streamServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        streamServer.send(204);
    });

    // ==== POST /register-success ====
    streamServer.on("/register-success", HTTP_POST, []() {
        start_fingerprint_enrollment_direct();

        streamServer.sendHeader("Access-Control-Allow-Origin", "*");
        streamServer.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        streamServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        streamServer.send(200, "application/json", "{\"success\":true}");
    });

    // ==== OPTIONS /attendance-success ====
    streamServer.on("/attendance-success", HTTP_OPTIONS, []() {
        streamServer.sendHeader("Access-Control-Allow-Origin", "*");
        streamServer.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        streamServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        streamServer.send(204);
    });

    // ==== POST /attendance-success ====
    streamServer.on("/attendance-success", HTTP_POST, []() {
        String body = streamServer.arg("plain");

        Serial.println("✅ Nhận tín hiệu điểm danh từ web");
        Serial.println("📦 Payload từ Web: " + body);

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            Serial.println("❌ Lỗi khi parse JSON");
            streamServer.sendHeader("Access-Control-Allow-Origin", "*");
            streamServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        String mssv = doc["mssv"] | "";
        String name = doc["name"] | "";
        String className = doc["className"] | "";
        studentClass = className;  // ✅ Lưu lại lớp vào biến toàn cục


        if (mssv == "" || name == "") {
            Serial.println("⚠️ Thiếu mssv hoặc name");
            streamServer.sendHeader("Access-Control-Allow-Origin", "*");
            streamServer.send(400, "application/json", "{\"error\":\"Thiếu dữ liệu\"}");
            return;
        }

        Serial.println("🎓 Mã số sinh viên: " + mssv);
        Serial.println("👤 Tên: " + name);
        Serial.println("🏫 Lớp: " + className);

        // 👉 Gọi hàm xử lý xác thực vân tay
        on_face_result_received(true, mssv, name,className);

        streamServer.sendHeader("Access-Control-Allow-Origin", "*");
        streamServer.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        streamServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        streamServer.send(200, "application/json", "{\"success\":true}");
    });

    // ==== STREAM ====
    streamServer.on("/stream", HTTP_GET, camera_stream_handler);
    streamServer.on("/capture", HTTP_GET, camera_capture_handler);

    // ==== KHỞI ĐỘNG SERVER ====
    streamServer.begin();
    isStreamActive = true;

    Serial.println("📡 Camera stream server started on port 81");
    Serial.println("▶️ Stream URL: http://" + WiFi.localIP().toString() + ":81/stream");
}


void stop_camera_stream_server() {
    isStreamActive = false;

    // Đóng tất cả kết nối hiện tại
    streamServer.close();   // Dừng tất cả client
    streamServer.stop();    // Dừng WebServer
    
    Serial.println("Camera stream server fully stopped");
}

// Handler cho route /stream
void camera_stream_handler() {
    WiFiClient client = streamServer.client();
    
    // Them CORS headers
    String header = "HTTP/1.1 200 OK\r\n";
    header += "Access-Control-Allow-Origin: *\r\n";
    header += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    client.print(header);
    
    while (client.connected() && isStreamActive) {
        // Kiểm tra heap trước khi capture
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 30000) {  // Nếu heap thấp hơn 30KB
            Serial.printf("⚠️ Heap thấp trong stream: %d bytes\n", freeHeap);
            delay(100);
            continue;
        }
        
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            delay(50);  // Tăng delay khi lỗi
            continue;
        }

        // Kiểm tra kích thước frame hợp lý
        if (fb->len > 0 && fb->len < 200000) {  // Giới hạn 200KB
            client.print("--frame\r\n");
            client.print("Content-Type: image/jpeg\r\n");
            client.printf("Content-Length: %u\r\n\r\n", fb->len);
            client.write(fb->buf, fb->len);
            client.print("\r\n");
        }

        esp_camera_fb_return(fb);
        fb = NULL;  // Đảm bảo không sử dụng lại

        delay(30);  // tốc Dộ stream
    }
}
void camera_capture_handler() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        streamServer.send(500, "text/plain", "Camera capture failed");
        return;
    }

    // Trả về ảnh với header phù hợp và cho phép CORS
    streamServer.sendHeader("Access-Control-Allow-Origin", "*");
    streamServer.sendHeader("Content-Type", "image/jpeg");
    streamServer.send_P(200, "image/jpeg", (char *)fb->buf, fb->len);

    esp_camera_fb_return(fb);
}

// Legacy function - giữ lai Dể tương thich với code cũ
void camera_stream_server_handle() {
    if (!isStreamActive) return;  // Nếu tat stream thì không xu ly
    
    // Nếu streamServer Dang chay, nó sẽ tự Dộng xu ly yeu cầu
    streamServer.handleClient();
}
void stream_task(void *param) {
    while (true) {
        camera_stream_server_handle();
        vTaskDelay(10 / portTICK_PERIOD_MS);  // giảm tải CPU
    }
}


void send_template_to_server(int id, const String &base64Data) {
    HTTPClient http;
    http.begin(serverURL + "/api/register-template");
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["id"] = id;  // MSSV thực tế
    doc["template"] = base64Data;

    String body;
    serializeJson(doc, body);

    int code = http.POST(body);
    http.end();

    if (code == 200) {
        Serial.println(" Gui template len server thanh cong");
        delay(500);
        check_student_status(currentMSSV);  // Kiểm tra lai trang thái sau khi gửi template
    } else {
        Serial.printf(" Gui that bai. Ma loi: %d\n", code);
    }
}
void send_register_signal_to_web(uint32_t mssv) {
    HTTPClient http;
    http.begin(serverURL + "/api/face-recognition/signal/face-register");
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["mssv"] = mssv;

    String body;
    serializeJson(doc, body);

    int code = http.POST(body);
    http.end();

    if (code == 200) {
        Serial.println(" Da gui tin hieu Dang ky face len web");
    } else {
        Serial.printf(" Loi gui tin hieu Dang ky face: %d\n", code);
    }
}

void handle_fingerprint_register_request() {
    Serial.println("📥 Web yeu cầu ESP32 bat Dầu Dang ky van tay");

    update_registration_ui("Dang ky khuon mat thanh cong", "Chuan bi dang ky van tay...", false);

    create_fingerprint_screen(FP_MODE_ENROLL_ONLY);
    switch_page(regScreen, fingerprintScreen);

    String msg = "Dang ky van tay cho MSSV " + String(currentMSSV);
    update_fingerprint_status(0, msg.c_str());

    start_fingerprint_enrollment();
}

// Thêm hàm gửi tín hiệu điểm danh đến web server
void send_attendance_signal_to_web() {
    HTTPClient http;
    http.begin(serverURL + "/api/face-recognition/signal/face-attendance");
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["signal"] = "attendance";
    doc["timestamp"] = getCurrentTimestamp();

    String body;
    serializeJson(doc, body);

    int code = http.POST(body);
    http.end();

    if (code == 200) {
        Serial.println("✅ Đã gửi tín hiệu điểm danh lên web");
    } else {
        Serial.printf("❌ Lỗi gửi tín hiệu điểm danh: %d\n", code);
    }
}

// Hàm trợ giúp để lấy timestamp hiện tại
String getCurrentTimestamp() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char timeString[50];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeString);
}

String get_template_by_mssv(const String& mssv) {
    HTTPClient http;
    String url = serverURL + "/api/face-recognition/fingerprint-template?id=" + mssv;
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode == 404) {
        Serial.println("⚠️ Không tìm thấy template vân tay cho MSSV này.");
        http.end();
        return "";
    }

    if (httpCode != 200) {
        Serial.printf("❌ Không thể lấy template từ server. Mã lỗi: %d\n", httpCode);
        http.end();
        return "";
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.println("❌ JSON lỗi hoặc template không đúng định dạng");
        return "";
    }

    if (!doc.containsKey("template")) {
        Serial.println("⚠️ JSON không chứa trường template");
        return "";
    }

    String tmpl = doc["template"].as<String>();
    if (tmpl.length() < 100) {
        Serial.println("⚠️ Template lấy được quá ngắn hoặc rỗng");
        return "";
    }

    Serial.println("✅ Template base64 lấy thành công từ server");
    return tmpl;
}


