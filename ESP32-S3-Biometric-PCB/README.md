# ESP32-S3 Biometric Attendance – Custom PCB Design

## Project Overview

This project presents a custom 2-layer PCB designed for a biometric attendance device based on ESP32-S3. The design integrates power management, high-speed SPI routing, and mixed-signal layout considerations in a compact form factor.

## Hardware Specifications

- **Layer Count**: 2-layer FR4
- **Copper Weight**: 1 oz copper
- **Board Size**: 70x44 mm
- **Min Trace/Space**: 0.15 mm
- **Min Drill**: 0.30 mm
- **Features**: Dedicated ground plane, controlled power routing.

## Power Design

- **Regulator**: Step-down buck converter (3.0-4.2V → 3.3V).
- **Optimization**: Minimized hot loop area, proper Cin placement, and thermal vias.

### Current & Trace Width Justification

The 3.3V rail is designed to support up to 1.5A load current, including:

- ESP32-S3 WiFi burst current
- TFT backlight
- Fingerprint sensor
- Safety design margin

Based on IPC-2221 guidelines (1 oz copper, 10°C temperature rise):

- Required trace width ≈ 0.8 mm
- Implemented trace width = 1.5 mm

This oversized width significantly reduces voltage drop and improves transient stability during high current spikes.

### Power Integrity Strategy

The TPS62130 synchronous buck converter operates at approximately 2.4 MHz switching frequency.

To ensure stable operation:

- Hot loop area minimized (VIN → SW → Inductor → CIN return)
- Input capacitor placed within a few millimeters of VIN and PGND
- Switching node trace kept as short as possible
- No signal routing under SW node
- Continuous ground reference plane maintained
- Thermal vias placed under regulator for heat spreading

### EMI Consideration

Due to high dv/dt and di/dt at the switching node:

- SW trace length minimized (~3 mm)
- Loop inductance reduced to limit ringing
- Ground return path kept compact
- ESP32 antenna keepout strictly respected

Increasing SW trace length (e.g., 15 mm) would increase loop inductance, resulting in higher radiated EMI and potential instability.

## RF Considerations

    - Antenna keepout region respected
    - No copper or ground plane under antenna
    - Controlled ground return near RF section
These measures preserve WiFi/BT performance.

## Manufacturing

**During DRC validation, the following were corrected**:
    - Hole clearance violations resolved
    - Drill size adjusted from 0.2 mm → 0.3 mm
    - Thermal relief optimization
    - Via stitching added for current spreading
    -Final DRC: clean before Gerber generation
The design is fully compatible with standard fabrication services (e.g., JLCPCB, PCBWay).

## Future Improvements
    - 4-layer stackup would improve EMI control and return path integrity
    - Additional decoupling optimization could further improve transient response
Additional decoupling optimization could further improve transient response