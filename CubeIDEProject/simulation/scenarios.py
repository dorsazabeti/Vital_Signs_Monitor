"""Physiologically meaningful vital-sign scenarios used by the simulator."""

from dataclasses import dataclass
from typing import Dict, Tuple


Range = Tuple[float, float]


@dataclass(frozen=True)
class Scenario:
    """Target ranges for one simulated patient condition."""

    heart_rate: Range
    spo2: Range
    temperature: Range


SCENARIOS: Dict[str, Scenario] = {
    "Normal": Scenario((68.0, 82.0), (96.0, 99.0), (36.5, 37.2)),
    "Tachycardia": Scenario((110.0, 140.0), (95.0, 99.0), (36.5, 37.3)),
    "Bradycardia": Scenario((42.0, 55.0), (95.0, 99.0), (36.4, 37.1)),
    "Low_SpO2": Scenario((85.0, 110.0), (84.0, 90.0), (36.5, 37.2)),
    "Abnormal_Temp": Scenario((90.0, 115.0), (95.0, 99.0), (38.3, 39.5)),
}


def get_scenario(name: str) -> Scenario:
    """Return a scenario or raise an error that lists valid names."""

    try:
        return SCENARIOS[name]
    except KeyError as exc:
        valid = ", ".join(SCENARIOS)
        raise ValueError(f"Unknown scenario: {name}. Valid scenarios: {valid}") from exc
