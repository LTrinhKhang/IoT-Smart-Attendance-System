# Biometric Attendance Firmware

This directory contains the ESP32-S3 firmware for the biometric attendance device. It handles user interaction via a touchscreen, biometric data processing, and communication with the central server.

## Features

- **Touchscreen UI**: Built with LVGL for a smooth and intuitive user experience.
- **Biometric Enrollment**: Supports fingerprint scanning and registration.
- **Camera Streaming**: Real-time camera feed for face recognition.
- **NTP Sync**: Automatic time synchronization via WiFi.
- **HTTP Client**: Seamless communication with the Node.js backend.

## Hardware Requirements

- **Microcontroller**: ESP32-S3 (with PSRAM enabled).
- **Display**: 3.5" or 4.3" TFT with capacitive touch.
- **Fingerprint Sensor**: R503 or compatible UART sensor.
- **Camera**: OV2640 or compatible module.

## Configuration

Before flashing, you must configure the following:

1.  **WiFi Credentials**: Usually configured through the UI on first boot or hardcoded in `wifi_ntp.cpp`.
2.  **Server IP**: Open `http_client.cpp` and update the `serverIP` and `serverPort` variables:
    ```cpp
    String serverIP = "YOUR_SERVER_IP"; 
    int serverPort = 5500;
    ```

## Installation

1.  Open the project in **Arduino IDE** or **VS Code with PlatformIO**.
2.  Install the required libraries:
    - `lvgl`
    - `ArduinoJson`
    - `TJpg_Decoder`
    - `ESP32 Camera`
3.  Select your board: **ESP32-S3 Dev Module**.
4.  Enable **PSRAM** in the Arduino IDE settings (OPI or QSPI depending on your module).
5.  Click **Upload**.

## File Structure

- `LVGL_PSRAM_1.ino`: Main entry point.
- `lvgl_ui.cpp/h`: UI screens and logic.
- `Finger.cpp/h`: Fingerprint sensor driver and logic.
- `http_client.cpp/h`: Backend communication.
- `camera_stream.cpp/h`: Camera initialization and frame handling.
