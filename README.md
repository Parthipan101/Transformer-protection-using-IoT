# ⚡ Transformer Protection System with ESP32 and Blynk

This project implements a **smart transformer protection and monitoring system** using an **ESP32**, featuring real-time control and monitoring via **Blynk**, along with safety mechanisms like **overcurrent** and **overtemperature** protection. It protects a 230V→12V transformer powering a DC motor load and supports manual and automatic control of relays.

---

## 🛠️ Features

- ✅ PWM-controlled dual DC motors (Motor A & B)
- ✅ Overcurrent protection (Relay-1)
- ✅ Overtemperature + IR-based protection (Relay-2)
- ✅ AC current estimation without CT
- ✅ Real-time monitoring with Blynk
- ✅ Manual override for relays
- ✅ Serial debug output

---

## 🧰 Hardware Components

- ESP32 Dev Board  
- 230V to 12V Transformer  
- Bridge Rectifier  
- 12V DC Hobby Motor  
- 2× Potentiometers (for PWM control)  
- LM35 Temperature Sensor  
- IR Obstacle Sensor  
- 2-Channel Relay Module  
- (Optional) ZMCT103 CT sensor – not used here due to signal issues

---

## 🔌 Wiring Overview

[230V AC] → [12V Transformer] → [Bridge Rectifier] → [Motor A & B]
↑
Controlled by ESP32 PWM
LM35 → ESP32
IR Sensor → ESP32
Relay1/Relay2 → Transformer Load/Motor Disconnect


---

## 📱 Blynk Dashboard Setup

| Virtual Pin | Purpose                    |
|-------------|----------------------------|
| V0          | Relay-1 manual control     |
| V1          | Relay-2 manual control     |
| V2          | Temperature (°C) display   |
| V3          | AC Current (A) display     |
| V4          | IR Obstacle status (Clear/Block) |

---

## 🔒 Protection Logic

- **Relay-1 (Overcurrent)**  
  Trips if estimated AC current ≥ 1.5 A

- **Relay-2 (Overtemperature / IR Clear)**  
  Trips if:
  - LM35 temperature ≥ 50°C  
  - OR IR beam is unblocked (i.e., no object present)

---

## ⚙️ Current Estimation Logic

Since ZMCT103 couldn't detect low current reliably, current is estimated based on motor PWM duty and impedance:

```c
I1_DC = (V_DC × dutyA) / MOTOR_IMPEDANCE;
I2_DC = (V_DC × dutyB) / MOTOR_IMPEDANCE;
I_total_DC = I1_DC + I2_DC;
I_AC ≈ (I_total_DC × V_DC) / (V_AC × Efficiency);
