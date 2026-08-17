import serial
import json
import time
import random
import math

class VitalSimulator:
    def __init__(self, port='COM3', baudrate=115200, scenario='Normal'):
        self.port = port
        self.baudrate = baudrate
        self.scenario = scenario
        self.time_t = 0.0
        
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"[*] Connected to {self.port}")
        except serial.SerialException:
            self.ser = None
            print("[!] UART not found. Running in MOCK mode (Console only).")

    def generate_ecg(self, heart_rate):
        beat_duration = 60.0 / heart_rate
        phase = self.time_t % beat_duration
        
        if phase < 0.1:
            return 1.5 * math.sin(phase * math.pi / 0.1)  # موج P
        elif 0.1 <= phase < 0.15:
            return -1.0                                   # موج Q
        elif 0.15 <= phase < 0.2:
            return 3.0                                    # موج R (پیک اصلی)
        elif 0.2 <= phase < 0.25:
            return -1.5                                   # موج S
        elif 0.4 <= phase < 0.6:
            return 0.8 * math.sin((phase - 0.4) * math.pi / 0.2)  # موج T
        else:
            return random.uniform(-0.1, 0.1)              # نویز طبیعی بیس‌لاین

    def get_data(self):
        if self.scenario == 'Normal':
            hr = random.randint(60, 100)
            spo2 = random.randint(95, 100)
            temp = round(random.uniform(36.5, 37.5), 1)
        elif self.scenario == 'Tachycardia': # تپش قلب بالا
            hr = random.randint(110, 150)
            spo2 = random.randint(95, 100)
            temp = round(random.uniform(36.5, 37.5), 1)
        elif self.scenario == 'Bradycardia': # تپش قلب پایین
            hr = random.randint(40, 55)
            spo2 = random.randint(95, 100)
            temp = round(random.uniform(36.5, 37.5), 1)
        elif self.scenario == 'Low_SpO2': 
            hr = random.randint(80, 110)
            spo2 = random.randint(80, 89)
            temp = round(random.uniform(36.5, 37.5), 1)
        elif self.scenario == 'Abnormal_Temp':
            hr = random.randint(90, 120)
            spo2 = random.randint(95, 100)
            temp = round(random.uniform(38.5, 40.0), 1)
        else:
            hr, spo2, temp = 72, 98, 37.0

        ecg = round(self.generate_ecg(hr), 3)
        self.time_t += 0.1 

        return {
            "scenario": self.scenario,
            "hr": hr,
            "spo2": spo2,
            "temp": temp,
            "ecg": ecg
        }

    def run(self):
        print(f"[*] Scenario: {self.scenario} | Press Ctrl+C to stop.")
        try:
            while True:
                data = self.get_data()
                json_str = json.dumps(data) + '\n'
                
                if self.ser and self.ser.is_open:
                    self.ser.write(json_str.encode('utf-8'))
                    print(f"Sent: {json_str.strip()}")
                else:
                    print(f"Mock: {json_str.strip()}")
                
                time.sleep(0.1)
                
        except KeyboardInterrupt:
            print("\n[!] Stopped.")
        finally:
            if self.ser and self.ser.is_open:
                self.ser.close()

if __name__ == "__main__":
    PORT = 'COM3' 
    BAUD_RATE = 115200
    SCENARIO = 'Normal' 
    
    sim = VitalSimulator(port=PORT, baudrate=BAUD_RATE, scenario=SCENARIO)
    sim.run()