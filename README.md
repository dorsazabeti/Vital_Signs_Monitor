# Vital Signs Monitor - IoT Simulator

## Project Overview
This repository contains the PC Simulator subsystem for an embedded Vital Signs Monitor, developed as part of the IoT Laboratory coursework. The overarching objective of the system is to acquire patient biometric data, render it in real-time via a TouchGFX-powered GUI on an STM32 microcontroller, and dispatch network-based alerts (Ethernet) under abnormal clinical conditions. 

In the current phase, prior to hardware sensor integration, this Python-based simulator is responsible for generating context-aware, real-time medical data (Heart Rate, SpO2, Body Temperature, and ECG signals).

## System Architecture
The project is decoupled into three primary modules:
1. **Data Generation (PC Simulator):** Simulates biometric data based on specific physiological scenarios (e.g., Normal, Tachycardia, Bradycardia, Hypoxemia) and serializes the output into JSON.
2. **Embedded Processing (STM32):** Parses incoming UART frames and drives the graphical user interface for real-time visualization.
3. **Network Alerting:** Monitors predefined thresholds to trigger Ethernet-based notifications (Email/Push).

## Communication Protocol
Data is transmitted using JSON over a USB-to-Serial UART bridge. 
* **Baud Rate:** 115200 bps
* **Data Frame Structure:** 
  ```json
  {"scenario": "string", "hr": int, "spo2": int, "temp": float, "ecg": float}
  ```

## Setup and Execution

### Prerequisites
* Python 3.8+
* USB-to-Serial Converter Hardware
* `pyserial` library

### Installation & Usage
Clone the repository and install the required serial communication package:
```bash
git clone [https://github.com/dorsazabeti/Vital_Signs_Monitor.git](https://github.com/dorsazabeti/Vital_Signs_Monitor.git)
cd Vital_Signs_Monitor
pip install pyserial
```

Execute the main simulator script:
```bash
python main.py
```
*Note: Prior to execution, ensure the `PORT` variable within `main.py` is configured to match your active UART bridge interface (e.g., `/dev/tty.usbserial-XXXX`).*

## Team and Responsibilities
* **Dorsa Zabeti:** Python simulator architecture, JSON data framing, vital signs data generation logic, and medical scenario modeling.
* **Arad Izadidoost:** STM32 board bring-up, UART communication handling, TouchGFX UI implementation, and real-time biometric visualization.
* **Yasaman Farrokhi:** Network protocol research, component integration testing, Ethernet notification implementation, and anomaly detection logic.