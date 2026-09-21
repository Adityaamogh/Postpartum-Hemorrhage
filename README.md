# Hemorrhage Whisperer: Wearable Early PPH Detection System

An IoT-enabled wearable medical monitoring system implemented on an **ESP32 microcontroller** designed for the early detection of **Postpartum Hemorrhage (PPH)**. 

The device combines multi-sensor physiological data collection (capacitive fluid monitoring, temperature tracking, and pulse oximetry) with a 4-state Finite State Machine (FSM) to provide real-time telemetry and immediate emergency alerting via **Blynk Cloud**.

---

## 🚀 Key Features & Hardware Specifications

* **Microcontroller:** ESP32 Board
* **Pulse Oximetry Sensor:** MAX30102 / MAX30105 (I2C interface) for continuous $\text{SpO}_2$ and PPG monitoring
* **Temperature Sensor:** DS18B20 digital thermal sensor (1-Wire bus on GPIO 4)
* **Fluid/Moisture Sensor:** Analog capacitive moisture sensor (GPIO 34) for blood accumulation detection
* **Alerting Mechanism:** Local audible alert via piezoelectric buzzer (GPIO 5) + cloud notifications
* **Cloud Telemetry:** Real-time data streaming and alert triggers via **Blynk IoT Platform** over Wi-Fi

---

## 🏗 Finite State Machine (FSM) Architecture

The firmware utilizes a robust 4-state state machine to prevent false alarms caused by patient movement or temporary sensor shifts:

```text
   +------------+
   | STATE_IDLE | <-----------------------------------------+
   +-----+------+                                           |
         | (Blood/Moisture detected & Sensor active)        |
         v                                                  |
+-------------------+                                       |
| STATE_CALIBRATING | (Calculates 4-second baseline SpO2)   | (Signal Lost or
+--------+----------+                                       |  Moisture Reset)
         |                                                  |
         v                                                  |
+-------------------+                                       |
|  STATE_MONITORING | --------------------------------------+
+--------+----------+
         | (SpO2 drops 5% below baseline)
         v
+-------------------+
|  STATE_EMERGENCY  | ---> Triggers Buzzer & Sends Blynk Cloud Alert
+-------------------+
