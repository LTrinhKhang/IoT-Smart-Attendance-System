#include "wifi_ntp.h"
#include "lvgl_ui.h"


// const char* ssid = "Qi";
// const char* password = "00000000";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "129.6.15.28", 7 * 3600, 60000);

// void connectWiFi() {
//     WiFi.begin(ssid, password);
//     Serial.print("Đang kết nối WiFi...");

//     unsigned long startTime = millis();
//     while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
//         delay(500);
//         Serial.print(".");
//     }

//     if (WiFi.status() == WL_CONNECTED) {
//         Serial.println("Đã kết nối WiFi!");
//     } else {
//         Serial.println("Không thể kết nối WiFi! Chuyển sang chế độ Offline...");
//     }
// }
void scanWifi(){
  int n = WiFi.scanNetworks();
  for (int i = 0; i<n; i++){
    Serial.printf("%d: %s (%d dBm)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }
}
void initNTP() {
    timeClient.begin();
    int attempts = 0;
    while (!timeClient.update() && attempts < 5) {
        Serial.println("⏳ Đang cập nhật thời gian từ NTP...");
        delay(1000);
        attempts++;
    }

    if (attempts < 5) {
        Serial.println("✅ NTP đã cập nhật thành công!");
    } else {
        Serial.println("❌ Không thể cập nhật thời gian từ NTP!");
    }
}



void update_time() {
    int hour = timeClient.getHours();
    int minute = timeClient.getMinutes();

    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", hour, minute);
    lv_label_set_text(clockLabel, timeStr);
    lv_obj_set_style_text_font(clockLabel, &lv_font_montserrat_48, LV_PART_MAIN);
}



