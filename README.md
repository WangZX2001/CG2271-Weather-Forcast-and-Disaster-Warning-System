# CG2271 Weather Forecast & Disaster Warning System

## Overview

The **Weather Forecast & Disaster Warning System** is an embedded IoT project developed for CG2271. The system is designed to monitor environmental conditions in real time and provide early warning alerts for potential disaster situations such as fire outbreaks, flooding, abnormal temperature changes, and physical shocks.

The project integrates:

- ESP32
- NXP MCXC444 Microcontroller
- FreeRTOS
- Multiple environmental sensors
- UART communication
- Cloud connectivity support

The system demonstrates concepts including:

- Real-time operating systems (RTOS)
- Multitasking
- Inter-task communication
- Interrupt Service Routines (ISR)
- Embedded communication protocols
- IoT sensor integration

---

## Features

### Real-Time Environmental Monitoring

The system continuously monitors:

- Temperature
- Humidity
- Water level
- Flame detection
- Physical shock/vibration

### Disaster Warning Alerts

The system triggers warnings using:

- Buzzers/Sirens
- RGB LEDs
- Serial terminal logs
- Cloud-based status updates

### FreeRTOS Multitasking

Implemented using multiple concurrent tasks:

- Sensor polling task
- UART communication task
- Buzzer control task
- Display/status task

### Bidirectional UART Communication

The ESP32 and MCXC444 communicate through UART to exchange:

- Temperature data
- Humidity data
- Water level readings
- Shock alerts
- Flame alerts

### Interrupt-Based Event Detection

Interrupt Service Routines (ISR) are used for:

- Shock detection
- Flame detection

This enables immediate response to critical events.

---

## System Architecture

### Hardware Components

| Component         | Purpose                              |
|-------------------|--------------------------------------|
| ESP32             | Main IoT controller and cloud communication |
| MCXC444           | Sensor processing and hardware control |
| DHT11             | Temperature and humidity sensing     |
| Flame Sensor      | Fire detection                       |
| Ultrasonic Sensor | Water level detection                |
| Shock Sensor      | Vibration/disaster event detection   |
| RGB LEDs          | Visual alert indicators              |
| Buzzer            | Audio warning system                 |

### Software Architecture

#### ESP32 Responsibilities

- Read DHT11 sensor data
- Manage FreeRTOS tasks
- Handle UART communication
- Trigger buzzer alerts
- Upload data to cloud services
- Control external RGB LEDs

#### MCXC444 Responsibilities

- Read ultrasonic sensor
- Detect flame events
- Detect shock events
- Handle ISR processing
- Control onboard RGB LEDs
- Send sensor data to ESP32

---

## Pin Configuration

### ESP32

| Component   | GPIO     |
|-------------|----------|
| DHT11       | GPIO 3   |
| Buzzer      | GPIO 4   |
| RGB Red     | GPIO 42  |
| RGB Green   | GPIO 41  |
| RGB Blue    | GPIO 40  |
| UART RX     | GPIO 18  |
| UART TX     | GPIO 17  |

### MCXC444

| Component          | Pin    |
|--------------------|--------|
| Shock Sensor       | PTC2   |
| Flame Sensor       | PTE20  |
| Ultrasonic Trigger | PTB0   |
| Ultrasonic Echo    | PTB1   |
| RGB Blue           | PTE29  |
| RGB Red            | PTE31  |
| RGB Green          | PTD5   |
| UART TX            | PTE22  |
| UART RX            | PTE23  |

---

## FreeRTOS Design

### Tasks

#### 1. DHT Sensor Task

- Periodically reads temperature and humidity
- Stores latest sensor readings into queue

#### 2. UART Send Task

- Sends formatted sensor data to MCXC444
- Protected using mutex synchronization

#### 3. UART Receive Task

- Receives warning messages from MCXC444
- Triggers buzzer events

#### 4. Buzzer Task

- Generates different warning sirens
- Handles multiple disaster conditions

### Synchronization Mechanisms

#### Queue

Used for:

- Passing sensor readings between tasks
- Managing buzzer events

#### Mutex

Used to:

- Prevent simultaneous UART access
- Ensure thread-safe serial communication

---

## UART Communication Protocol

### ESP32 → MCXC444

Example:

```
TEMP:30.5,HUM:78.2
```

### MCXC444 → ESP32

**Shock Detection:**

```
SHOCK
```

**Water Level:**

```
DIST:12.3
```

**Flame Detection:**

```
FLAME
```

---

## Alert System

### Temperature Status

| Temperature Range | LED Status |
|-------------------|------------|
| Low               | Blue       |
| Normal            | Green      |
| High              | Red        |

### Disaster Alerts

| Event            | Alert              |
|------------------|--------------------|
| Shock            | Siren              |
| Flame            | Continuous buzzer  |
| Flood Risk       | Warning buzzer     |
| High Temperature | Red LED            |

---

## Cloud Integration

The ESP32 supports cloud connectivity for:

- Remote monitoring
- Sensor data logging
- Disaster notifications

Potential platforms:

- Firebase
- MQTT
- IoT dashboards

---

## Development Tools

| Tool           | Usage                        |
|----------------|------------------------------|
| MCUXpresso IDE | MCXC444 development          |
| Arduino IDE    | ESP32 development            |
| FreeRTOS       | Real-time task scheduling    |
| GitHub         | Version control              |

---

## Setup Instructions

### ESP32 Setup

1. Install Arduino IDE
2. Install ESP32 board package
3. Install required libraries:
   - DHT sensor library
   - FreeRTOS support
4. Upload firmware

### MCXC444 Setup

1. Open MCUXpresso IDE
2. Import project
3. Build firmware
4. Flash to board

---

## Future Improvements

- Machine learning prediction models
- Mobile application integration
- Real-time weather forecasting APIs
- GPS location tracking
- SMS emergency notifications
- Battery backup system
- Solar-powered deployment

---

## Learning Outcomes

This project demonstrates:

- Embedded systems development
- RTOS task scheduling
- Hardware-software integration
- UART communication
- Interrupt programming
- IoT system design
- Real-time monitoring systems

---

## Contributors

Developed as part of the NUS CG2271 Embedded Systems project.

---

## License

This project is for educational purposes under NUS CG2271 coursework.
