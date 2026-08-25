import json
import math
import random
import time

import serial
from serial.tools import list_ports


SCENARIOS = {
    "Normal": {
        "hr": (68.0, 82.0),
        "spo2": (96.0, 99.0),
        "temp": (36.5, 37.2),
    },
    "Tachycardia": {
        "hr": (110.0, 140.0),
        "spo2": (95.0, 99.0),
        "temp": (36.5, 37.3),
    },
    "Bradycardia": {
        "hr": (42.0, 55.0),
        "spo2": (95.0, 99.0),
        "temp": (36.4, 37.1),
    },
    "Low_SpO2": {
        "hr": (85.0, 110.0),
        "spo2": (84.0, 90.0),
        "temp": (36.5, 37.2),
    },
    "Abnormal_Temp": {
        "hr": (90.0, 115.0),
        "spo2": (95.0, 99.0),
        "temp": (38.3, 39.5),
    },
}


class VitalSimulator:
    def __init__(
        self,
        port=None,
        baudrate=115200,
        scenario="Normal",
        sample_interval=0.02,
        target_update_interval=4.0,
        console_interval=0.5,
    ):
        if scenario not in SCENARIOS:
            raise ValueError(
                f"Unknown scenario: {scenario}. "
                f"Valid scenarios: {', '.join(SCENARIOS)}"
            )

        self.port = port
        self.baudrate = baudrate
        self.scenario = scenario
        self.sample_interval = sample_interval
        self.target_update_interval = target_update_interval
        self.console_interval = console_interval

        self.ser = None

        self.current_hr = 74.0
        self.current_spo2 = 98.0
        self.current_temp = 36.8

        self.target_hr = self.current_hr
        self.target_spo2 = self.current_spo2
        self.target_temp = self.current_temp

        self.ecg_phase = 0.0
        self.elapsed_time = 0.0
        self.target_elapsed = target_update_interval
        self.console_elapsed = console_interval

        self._connect_serial()
        self._choose_new_targets()

    def _connect_serial(self):
        if not self.port:
            print("[*] No UART port selected. Running in MOCK mode.")
            self._print_available_ports()
            return

        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1,
                write_timeout=1,
            )
            time.sleep(0.2)
            print(f"[*] Connected to {self.port} @ {self.baudrate} baud")
        except (serial.SerialException, OSError) as exc:
            self.ser = None
            print(f"[!] Could not open UART port {self.port}: {exc}")
            print("[*] Running in MOCK mode.")
            self._print_available_ports()

    @staticmethod
    def _available_ports():
        return [port.device for port in list_ports.comports()]

    def _print_available_ports(self):
        ports = self._available_ports()
        if ports:
            print("[*] Available serial ports:")
            for port in ports:
                print(f"    {port}")
        else:
            print("[*] No serial ports detected.")

    @staticmethod
    def _clamp(value, low, high):
        return max(low, min(high, value))

    @staticmethod
    def _random_target(low, high):
        center = (low + high) / 2.0
        return random.triangular(low, high, center)

    @staticmethod
    def _move_towards(current, target, max_delta):
        difference = target - current

        if abs(difference) <= max_delta:
            return target

        if difference > 0:
            return current + max_delta

        return current - max_delta

    @staticmethod
    def _inside(value, limits):
        low, high = limits
        return low <= value <= high

    def _choose_new_targets(self):
        config = SCENARIOS[self.scenario]

        self.target_hr = self._random_target(*config["hr"])
        self.target_spo2 = self._random_target(*config["spo2"])
        self.target_temp = self._random_target(*config["temp"])

        self.target_elapsed = 0.0

    def set_scenario(self, scenario):
        if scenario not in SCENARIOS:
            raise ValueError(
                f"Unknown scenario: {scenario}. "
                f"Valid scenarios: {', '.join(SCENARIOS)}"
            )

        if scenario == self.scenario:
            return

        self.scenario = scenario
        self._choose_new_targets()
        print(f"[*] Scenario changed to: {self.scenario}")

    def _update_vitals(self, dt):
        config = SCENARIOS[self.scenario]

        self.target_elapsed += dt
        if self.target_elapsed >= self.target_update_interval:
            self._choose_new_targets()

        hr_rate = 1.2 if self._inside(self.current_hr, config["hr"]) else 6.0
        spo2_rate = 0.18 if self._inside(self.current_spo2, config["spo2"]) else 1.2
        temp_rate = 0.015 if self._inside(self.current_temp, config["temp"]) else 0.15

        self.current_hr = self._move_towards(
            self.current_hr,
            self.target_hr,
            hr_rate * dt,
        )
        self.current_spo2 = self._move_towards(
            self.current_spo2,
            self.target_spo2,
            spo2_rate * dt,
        )
        self.current_temp = self._move_towards(
            self.current_temp,
            self.target_temp,
            temp_rate * dt,
        )

        self.current_hr += random.gauss(0.0, 0.03)
        self.current_spo2 += random.gauss(0.0, 0.004)
        self.current_temp += random.gauss(0.0, 0.0005)

        hr_low, hr_high = config["hr"]
        spo2_low, spo2_high = config["spo2"]
        temp_low, temp_high = config["temp"]

        hr_margin = 2.0
        spo2_margin = 0.5
        temp_margin = 0.1

        self.current_hr = self._clamp(
            self.current_hr,
            hr_low - hr_margin,
            hr_high + hr_margin,
        )
        self.current_spo2 = self._clamp(
            self.current_spo2,
            spo2_low - spo2_margin,
            spo2_high + spo2_margin,
        )
        self.current_temp = self._clamp(
            self.current_temp,
            temp_low - temp_margin,
            temp_high + temp_margin,
        )

    @staticmethod
    def _gaussian(x, center, width, amplitude):
        distance = x - center
        return amplitude * math.exp(
            -(distance * distance) / (2.0 * width * width)
        )

    def generate_ecg(self, heart_rate, dt):
        safe_hr = max(30.0, heart_rate)
        beat_duration = 60.0 / safe_hr

        self.ecg_phase = (
            self.ecg_phase + (dt / beat_duration)
        ) % 1.0

        phase = self.ecg_phase

        p_wave = self._gaussian(phase, 0.18, 0.025, 0.12)
        q_wave = self._gaussian(phase, 0.34, 0.012, -0.15)
        r_wave = self._gaussian(phase, 0.37, 0.010, 1.20)
        s_wave = self._gaussian(phase, 0.40, 0.015, -0.25)
        t_wave = self._gaussian(phase, 0.65, 0.055, 0.35)

        baseline = 0.02 * math.sin(
            2.0 * math.pi * 0.33 * self.elapsed_time
        )
        noise = random.gauss(0.0, 0.008)

        return (
            p_wave
            + q_wave
            + r_wave
            + s_wave
            + t_wave
            + baseline
            + noise
        )

    def get_data(self, dt=None):
        if dt is None:
            dt = self.sample_interval

        dt = self._clamp(dt, 0.001, 0.2)

        self.elapsed_time += dt
        self._update_vitals(dt)

        ecg = self.generate_ecg(self.current_hr, dt)

        return {
            "scenario": self.scenario,
            "hr": int(round(self.current_hr)),
            "spo2": int(round(self.current_spo2)),
            "temp": round(self.current_temp, 1),
            "ecg": round(ecg, 3),
        }

    def _send(self, payload):
        json_line = json.dumps(
            payload,
            separators=(",", ":"),
        ) + "\n"

        if self.ser and self.ser.is_open:
            try:
                self.ser.write(json_line.encode("utf-8"))
                return "Sent", json_line
            except (serial.SerialException, OSError) as exc:
                print(f"[!] UART write failed: {exc}")
                try:
                    self.ser.close()
                except serial.SerialException:
                    pass
                self.ser = None
                print("[*] Switched to MOCK mode.")

        return "Mock", json_line

    def run(self):
        print(f"[*] Scenario: {self.scenario}")
        print("[*] Press Ctrl+C to stop.")

        last_tick = time.monotonic()
        next_tick = last_tick

        try:
            while True:
                now = time.monotonic()
                dt = now - last_tick
                last_tick = now

                data = self.get_data(dt)
                mode, json_line = self._send(data)

                self.console_elapsed += dt
                if self.console_elapsed >= self.console_interval:
                    print(f"{mode}: {json_line.strip()}")
                    self.console_elapsed = 0.0

                next_tick += self.sample_interval
                sleep_time = next_tick - time.monotonic()

                if sleep_time > 0:
                    time.sleep(sleep_time)
                else:
                    next_tick = time.monotonic()

        except KeyboardInterrupt:
            print("\n[!] Stopped.")

        finally:
            if self.ser and self.ser.is_open:
                self.ser.close()


if __name__ == "__main__":
    PORT = None
    BAUD_RATE = 115200
    SCENARIO = "Normal"

    simulator = VitalSimulator(
        port=PORT,
        baudrate=BAUD_RATE,
        scenario=SCENARIO,
        sample_interval=0.02,
        target_update_interval=4.0,
        console_interval=0.5,
    )

    simulator.run()
