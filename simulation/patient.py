"""Patient physiology and ECG signal generation."""

import math
import random
from typing import Dict, Optional

from .scenarios import get_scenario


class PatientModel:
    """Generate smooth vital signs for a selectable patient scenario."""

    PHYSIOLOGICAL_LIMITS = {
        "hr": (30.0, 220.0),
        "spo2": (50.0, 100.0),
        "temp": (32.0, 43.0),
    }

    def __init__(
        self,
        scenario: str = "Normal",
        target_update_interval: float = 4.0,
        rng: Optional[random.Random] = None,
    ) -> None:
        if target_update_interval <= 0:
            raise ValueError("target_update_interval must be positive")

        self.rng = rng or random.Random()
        self.scenario_name = scenario
        self.scenario = get_scenario(scenario)
        self.target_update_interval = target_update_interval

        self.current_hr = self._random_target(self.scenario.heart_rate)
        self.current_spo2 = self._random_target(self.scenario.spo2)
        self.current_temp = self._random_target(self.scenario.temperature)
        self.target_hr = self.current_hr
        self.target_spo2 = self.current_spo2
        self.target_temp = self.current_temp

        self.ecg_phase = 0.0
        self.elapsed_time = 0.0
        self.target_elapsed = target_update_interval
        self._choose_new_targets()

    @staticmethod
    def _clamp(value: float, low: float, high: float) -> float:
        return max(low, min(high, value))

    def _random_target(self, limits) -> float:
        low, high = limits
        return self.rng.triangular(low, high, (low + high) / 2.0)

    @staticmethod
    def _move_towards(current: float, target: float, max_delta: float) -> float:
        difference = target - current
        if abs(difference) <= max_delta:
            return target
        return current + math.copysign(max_delta, difference)

    @staticmethod
    def _inside(value: float, limits) -> bool:
        low, high = limits
        return low <= value <= high

    def _choose_new_targets(self) -> None:
        self.target_hr = self._random_target(self.scenario.heart_rate)
        self.target_spo2 = self._random_target(self.scenario.spo2)
        self.target_temp = self._random_target(self.scenario.temperature)
        self.target_elapsed = 0.0

    def set_scenario(self, scenario: str) -> None:
        """Change condition while keeping the patient's values continuous."""

        new_scenario = get_scenario(scenario)
        if scenario == self.scenario_name:
            return
        self.scenario_name = scenario
        self.scenario = new_scenario
        self._choose_new_targets()

    def _update_vitals(self, dt: float) -> None:
        self.target_elapsed += dt
        if self.target_elapsed >= self.target_update_interval:
            self._choose_new_targets()

        # A patient already inside the requested range varies slowly. A scenario
        # transition is faster, but remains continuous and visible on the chart.
        hr_rate = 1.2 if self._inside(self.current_hr, self.scenario.heart_rate) else 8.0
        spo2_rate = 0.18 if self._inside(self.current_spo2, self.scenario.spo2) else 2.0
        temp_rate = 0.015 if self._inside(self.current_temp, self.scenario.temperature) else 0.35

        self.current_hr = self._move_towards(self.current_hr, self.target_hr, hr_rate * dt)
        self.current_spo2 = self._move_towards(self.current_spo2, self.target_spo2, spo2_rate * dt)
        self.current_temp = self._move_towards(self.current_temp, self.target_temp, temp_rate * dt)

        self.current_hr += self.rng.gauss(0.0, 0.03)
        self.current_spo2 += self.rng.gauss(0.0, 0.004)
        self.current_temp += self.rng.gauss(0.0, 0.0005)

        self.current_hr = self._clamp(self.current_hr, *self.PHYSIOLOGICAL_LIMITS["hr"])
        self.current_spo2 = self._clamp(self.current_spo2, *self.PHYSIOLOGICAL_LIMITS["spo2"])
        self.current_temp = self._clamp(self.current_temp, *self.PHYSIOLOGICAL_LIMITS["temp"])

    @staticmethod
    def _gaussian(x: float, center: float, width: float, amplitude: float) -> float:
        distance = x - center
        return amplitude * math.exp(-(distance * distance) / (2.0 * width * width))

    def _generate_ecg(self, heart_rate: float, dt: float) -> float:
        beat_duration = 60.0 / max(30.0, heart_rate)
        self.ecg_phase = (self.ecg_phase + dt / beat_duration) % 1.0
        phase = self.ecg_phase
        signal = (
            self._gaussian(phase, 0.18, 0.025, 0.12)
            + self._gaussian(phase, 0.34, 0.012, -0.15)
            + self._gaussian(phase, 0.37, 0.010, 1.20)
            + self._gaussian(phase, 0.40, 0.015, -0.25)
            + self._gaussian(phase, 0.65, 0.055, 0.35)
        )
        baseline = 0.02 * math.sin(2.0 * math.pi * 0.33 * self.elapsed_time)
        noise = self.rng.gauss(0.0, 0.008)
        return signal + baseline + noise

    def sample(self, dt: float) -> Dict[str, object]:
        """Advance the model and return one JSON-compatible sample."""

        if dt <= 0:
            raise ValueError("dt must be positive")
        dt = self._clamp(dt, 0.001, 0.2)
        self.elapsed_time += dt
        self._update_vitals(dt)
        ecg = self._generate_ecg(self.current_hr, dt)
        return {
            "scenario": self.scenario_name,
            "hr": int(round(self.current_hr)),
            "spo2": int(round(self.current_spo2)),
            "temp": round(self.current_temp, 1),
            "ecg": round(ecg, 3),
        }
