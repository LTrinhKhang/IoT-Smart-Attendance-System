#ifndef WIFI_NTP_H
#define WIFI_NTP_H

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

extern const char* ssid;
extern const char* password;
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

// void connectWiFi();
void initNTP();
void update_time();
void scanWifi();

#endif
