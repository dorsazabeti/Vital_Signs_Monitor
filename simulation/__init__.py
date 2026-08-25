"""PC-side vital-sign simulator for the IoT lab project."""

from .main import VitalSimulator
from .patient import PatientModel
from .scenarios import SCENARIOS, Scenario
from .uart import UARTTransport, encode_frame

__all__ = [
    "PatientModel",
    "SCENARIOS",
    "Scenario",
    "UARTTransport",
    "VitalSimulator",
    "encode_frame",
]
