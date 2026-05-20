Smart Crutch – Embedded Assistive System (Wokwi Simulation)

Date: 9 Feb 2025
Platform: ESP32 (simulated on Wokwi)
Language: Embedded C / Arduino framework

Overview
This project explores a simulated embedded system for a smart crutch intended to assist mobility-impaired users through basic sensing, safety detection, and health monitoring.

The system integrates load sensing, motion tracking, fall detection logic, vital monitoring (mocked), battery simulation, and BLE-style packet transmission. The goal was to understand how hardware sensing, firmware logic, and user-facing signals can be integrated in a human-centred assistive device.

This project was developed as part of exploratory product design work.

System Architecture:
Sensors Integrated
HX711 Load Cell Amplifier - Measures weight applied to the crutch.
MPU6050 (Accelerometer + Gyroscope) - Computes pitch, roll, and acceleration magnitude for:
posture tracking
fall detection
MAX30102 (Mocked in simulation)
Simulated heart rate and SpO2 readings.
Battery + Charging Simulation
Models charge/discharge behaviour.

Core Functionalities
1. Load Monitoring

Continuously samples HX711 and stores last measured weight.

2. Orientation & Fall Detection

Using accelerometer data:
Calculates pitch and roll.
Computes acceleration magnitude.
Triggers fall flag if magnitude crosses thresholds.

Fall condition:
accMag < 3  (free fall)
accMag > 25 (impact)

3. SOS Trigger

Push-button interrupt logic with debounce handling.

4. Battery Simulation

Gradual discharge when not charging.
Gradual charge when USB connected.
Battery percentage transmitted in BLE packet.

5. BLE Packet Simulation

Data structured into a CrutchPacket and printed as JSON-style output:

{
  "load": ...,
  "pitch": ...,
  "roll": ...,
  "accMag": ...,
  "HR": ...,
  "SpO2": ...,
  "battery": ...,
  "fall": ...,
  "sos": ...
}

Design Considerations
Modular sensor reading functions
Time-based loop control (3-second update cycle)
Continuous sampling for critical sensors (HX711)
Simulated vitals to allow full system testing
No blocking delays in main loop
Threshold-based event detection

Limitations
Vitals sensor is mocked (randomised values)
BLE transmission simulated via serial output
Fall detection uses simple threshold logic (no filtering)
No real-world calibration performed
No power optimization logic

Future Extensions
Implement low-pass filtering for IMU data
Replace threshold fall detection with ML-based classification
Add real MAX30102 integration
Implement real BLE communication stack
Add step-counting and gait pattern analysis

Learning Outcomes
This project helped develop understanding of:
Sensor fusion at embedded level
Structuring firmware for real-time updates
Translating human-centred product ideas into system logic
Handling boundary cases in embedded systems
Simulation-driven prototyping before hardware deployment
