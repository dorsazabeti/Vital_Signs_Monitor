"""Newline-delimited JSON transport over UART, with a safe mock mode."""

import json
import time
from typing import Dict, List, Optional, Tuple

try:
    import serial as pyserial
    from serial.tools import list_ports

    if not hasattr(pyserial, "serial_for_url"):
        raise ImportError("The installed serial package is not PySerial")
except (ImportError, ModuleNotFoundError):
    pyserial = None
    list_ports = None


def available_ports() -> List[str]:
    if list_ports is None:
        return []
    return [port.device for port in list_ports.comports()]


def encode_frame(payload: Dict[str, object]) -> bytes:
    """Encode one compact UTF-8 JSON object terminated by a newline."""

    line = json.dumps(payload, separators=(",", ":"), allow_nan=False) + "\n"
    return line.encode("utf-8")


class UARTTransport:
    """Write framed samples to serial, or report them without hardware."""

    def __init__(
        self,
        port: Optional[str] = None,
        baudrate: int = 115200,
        reconnect_interval: float = 2.0,
    ) -> None:
        if baudrate <= 0:
            raise ValueError("baudrate must be positive")
        if reconnect_interval < 0:
            raise ValueError("reconnect_interval cannot be negative")
        self.port = port
        self.baudrate = baudrate
        self.reconnect_interval = reconnect_interval
        self.serial = None
        self.last_error: Optional[str] = None
        self._next_reconnect = 0.0

    @property
    def is_connected(self) -> bool:
        return bool(self.serial and self.serial.is_open)

    def connect(self) -> bool:
        if not self.port or pyserial is None:
            if self.port and pyserial is None:
                self.last_error = "PySerial is not installed"
            return False
        try:
            # serial_for_url supports normal device paths and PySerial test URLs
            # such as loop://, which lets us verify framing without a board.
            self.serial = pyserial.serial_for_url(
                self.port,
                baudrate=self.baudrate,
                timeout=1,
                write_timeout=1,
            )
            time.sleep(0.2)
            self.last_error = None
            return True
        except (pyserial.SerialException, OSError) as exc:
            self.serial = None
            self.last_error = str(exc)
            self._next_reconnect = time.monotonic() + self.reconnect_interval
            return False

    def send(self, payload: Dict[str, object]) -> Tuple[str, str]:
        frame = encode_frame(payload)
        if (
            not self.is_connected
            and self.port
            and time.monotonic() >= self._next_reconnect
        ):
            self.connect()
        if self.is_connected:
            try:
                self.serial.write(frame)
                return "Sent", frame.decode("utf-8")
            except (pyserial.SerialException, OSError) as exc:
                self.last_error = str(exc)
                self.close()
                self._next_reconnect = time.monotonic() + self.reconnect_interval
        return "Mock", frame.decode("utf-8")

    def close(self) -> None:
        if self.serial:
            try:
                if self.serial.is_open:
                    self.serial.close()
            except (OSError, AttributeError):
                pass
        self.serial = None

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.close()
