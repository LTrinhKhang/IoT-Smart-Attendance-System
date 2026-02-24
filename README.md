# IoT Biometric Attendance System

Full-stack IoT biometric attendance system integrating custom PCB hardware, ESP32-S3 firmware, and a centralized Node.js backend.

This project demonstrates system-level engineering across hardware design, embedded development, and backend architecture.

---

## System Overview

The system consists of three major components:

1. **Hardware – ESP32-S3-Biometric-PCB**
   - Custom 2-layer PCB
   - TPS62130 synchronous buck regulator
   - ESP32-S3-WROOM module
   - TFT SPI display
   - Optical fingerprint sensor
   - Li-ion powered architecture

2. **Firmware – ESP32-S3-Firmware-Biometric**
   - Written in C++ (Arduino framework)
   - LVGL-based touchscreen UI
   - Fingerprint template handling (Base64 encoding/decoding)
   - Camera streaming support
   - HTTP client + WebSocket communication
   - NTP time synchronization

3. **Backend – Biometric-Attendance-Server**
   - Node.js + Express REST API
   - MySQL database
   - Real-time attendance logging
   - Face recognition (128D embedding)
   - Web dashboard for management and reporting

---

## High-Level Architecture

Edge Device (ESP32-S3)
→ HTTP REST / WebSocket
→ Node.js Server
→ MySQL Database

Biometric workflow:
- Face recognition using vector comparison
- Fingerprint verification via template matching
- Attendance event logging with timestamp

---

## Engineering Scope

This project covers:

- Custom PCB design with power integrity considerations
- High-frequency buck layout optimization (2.4 MHz switching)
- EMI mitigation strategies
- Embedded firmware architecture and state machine control
- Biometric data processing and encoding
- Backend API and database schema design
- Distributed IoT communication model

---

## Project Structure
- ESP32-S3-Biometric-PCB/ # Hardware design (KiCad + Gerber)
- ESP32-S3-Firmware-Biometric/ # ESP32 firmware
- Biometric-Attendance-Server/ # Node.js backend
- images/ # PCB renders and system illustrations

---

## Key Technical Highlights

- 3.3V rail designed for 1.5A peak load
- Oversized power trace for reduced IR drop
- Minimized buck hot-loop area
- Thermal vias under regulator
- Antenna keepout respected
- DRC-clean and fabrication-ready Gerber

---

## Setup Overview

### Hardware
Refer to:
`ESP32-S3-Biometric-PCB/README.md`

### Firmware
- Configure WiFi credentials
- Set server IP and port
- Flash using Arduino IDE or PlatformIO

### Server
- Install Node.js (v16+)
- Configure `.env`
- Run:  npm install
        node server/index.js

---

## Future Improvements

- Migration to 4-layer PCB for improved EMI control
- Enhanced transient response validation
- TLS-secured communication
- OTA firmware updates

---

## License

MIT (or specify applicable license)