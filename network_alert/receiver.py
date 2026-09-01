"""Receive, validate, display, log, and optionally forward STM32 UDP alerts."""

from __future__ import annotations

import argparse
import json
import platform
import socket
import subprocess
import sys
import urllib.error
import urllib.request
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


VALID_EVENTS = {"triggered", "reminder", "recovered"}
VALID_SEVERITIES = {"normal", "warning", "critical"}
VALID_REASONS = {
    "low_heart_rate",
    "high_heart_rate",
    "low_spo2",
    "low_temperature",
    "high_temperature",
}


class AlertValidationError(ValueError):
    """Raised when a received datagram is not a valid vital-alert payload."""


@dataclass(frozen=True)
class AlertMessage:
    event: str
    sequence: int
    uptime_ms: int
    severity: str
    scenario: str
    heart_rate: int
    spo2: int
    temperature_c: float
    reasons: tuple[str, ...]

    @property
    def is_alarm(self) -> bool:
        return self.event != "recovered"

    def summary(self) -> str:
        state = {
            "triggered": "هشدار جدید",
            "reminder": "یادآوری هشدار",
            "recovered": "بازگشت به وضعیت عادی",
        }[self.event]
        reason_text = ", ".join(self.reasons) if self.reasons else "normal"
        return (
            f"{state} | {self.scenario} | HR={self.heart_rate} | "
            f"SpO2={self.spo2}% | Temp={self.temperature_c:.1f} C | "
            f"reasons={reason_text} | seq={self.sequence}"
        )


def _require_int(payload: dict[str, Any], key: str, minimum: int, maximum: int) -> int:
    value = payload.get(key)
    if isinstance(value, bool) or not isinstance(value, int):
        raise AlertValidationError(f"{key} must be an integer")
    if not minimum <= value <= maximum:
        raise AlertValidationError(f"{key} is outside {minimum}..{maximum}")
    return value


def parse_alert(datagram: bytes) -> AlertMessage:
    """Decode and validate one board datagram without trusting its contents."""
    if len(datagram) > 1024:
        raise AlertValidationError("payload is too large")

    try:
        payload = json.loads(datagram.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise AlertValidationError("payload is not valid UTF-8 JSON") from exc

    if not isinstance(payload, dict):
        raise AlertValidationError("payload root must be an object")
    if payload.get("version") != 1 or payload.get("type") != "vital_alert":
        raise AlertValidationError("unsupported alert protocol")

    event = payload.get("event")
    severity = payload.get("severity")
    scenario = payload.get("scenario")
    reasons = payload.get("reasons")
    if event not in VALID_EVENTS:
        raise AlertValidationError("unknown event")
    if severity not in VALID_SEVERITIES:
        raise AlertValidationError("unknown severity")
    if not isinstance(scenario, str) or not scenario or len(scenario) > 23:
        raise AlertValidationError("scenario must be 1..23 characters")
    if not isinstance(reasons, list) or not all(reason in VALID_REASONS for reason in reasons):
        raise AlertValidationError("reasons contains an unknown value")
    if len(reasons) != len(set(reasons)):
        raise AlertValidationError("reasons must not contain duplicates")

    temperature = payload.get("temperature_c")
    if isinstance(temperature, bool) or not isinstance(temperature, (int, float)):
        raise AlertValidationError("temperature_c must be numeric")
    if not -20.0 <= float(temperature) <= 60.0:
        raise AlertValidationError("temperature_c is outside the accepted range")

    message = AlertMessage(
        event=event,
        sequence=_require_int(payload, "sequence", 1, 0xFFFFFFFF),
        uptime_ms=_require_int(payload, "uptime_ms", 0, 0xFFFFFFFF),
        severity=severity,
        scenario=scenario,
        heart_rate=_require_int(payload, "heart_rate", 0, 300),
        spo2=_require_int(payload, "spo2", 0, 100),
        temperature_c=float(temperature),
        reasons=tuple(reasons),
    )
    if message.event == "recovered" and (message.severity != "normal" or message.reasons):
        raise AlertValidationError("recovered events must be normal and have no reasons")
    if message.event != "recovered" and (message.severity == "normal" or not message.reasons):
        raise AlertValidationError("active alerts must include abnormal severity and reasons")
    return message


class SequenceDeduplicator:
    def __init__(self) -> None:
        self._last_by_sender: dict[str, tuple[int, int]] = {}

    def is_duplicate(self, sender: str, message: AlertMessage) -> bool:
        previous = self._last_by_sender.get(sender)
        current = (message.uptime_ms, message.sequence)
        if previous is not None:
            previous_uptime, previous_sequence = previous
            same_boot = message.uptime_ms >= previous_uptime
            if same_boot and message.sequence <= previous_sequence:
                return True
        self._last_by_sender[sender] = current
        return False


def _desktop_notification(message: AlertMessage) -> None:
    if platform.system() != "Darwin":
        return
    title = "Vital Monitor: " + ("ALERT" if message.is_alarm else "RECOVERED")
    body = message.summary().replace("\\", "\\\\").replace('"', '\\"')
    safe_title = title.replace("\\", "\\\\").replace('"', '\\"')
    script = f'display notification "{body}" with title "{safe_title}"'
    subprocess.run(
        ["osascript", "-e", script],
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        timeout=5,
    )


def _append_log(path: Path, raw_payload: dict[str, Any], sender: str) -> None:
    record = {
        "received_at": datetime.now(timezone.utc).isoformat(),
        "sender": sender,
        **raw_payload,
    }
    with path.open("a", encoding="utf-8") as log_file:
        json.dump(record, log_file, ensure_ascii=False, separators=(",", ":"))
        log_file.write("\n")


def _forward_webhook(url: str, datagram: bytes, timeout: float) -> None:
    request = urllib.request.Request(
        url,
        data=datagram,
        headers={"Content-Type": "application/json", "User-Agent": "VitalMonitor/1"},
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        if not 200 <= response.status < 300:
            raise urllib.error.HTTPError(
                url, response.status, "webhook returned an error", response.headers, None
            )


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Receive UDP vital alerts from the STM32 Ethernet module."
    )
    parser.add_argument("--bind", default="0.0.0.0", help="local address (default: all)")
    parser.add_argument("--port", type=int, default=5055, help="UDP port (default: 5055)")
    parser.add_argument("--log", type=Path, help="append validated alerts as JSON Lines")
    parser.add_argument("--webhook-url", help="forward each validated alert with HTTP POST")
    parser.add_argument("--webhook-timeout", type=float, default=5.0)
    parser.add_argument(
        "--notify",
        action=argparse.BooleanOptionalAction,
        default=platform.system() == "Darwin",
        help="show macOS desktop notifications (enabled by default on macOS)",
    )
    return parser


def run_receiver(args: argparse.Namespace) -> int:
    if not 1 <= args.port <= 65535:
        raise SystemExit("port must be in 1..65535")
    deduplicator = SequenceDeduplicator()

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server:
        server.bind((args.bind, args.port))
        print(f"Listening for STM32 alerts on {args.bind}:{args.port}", flush=True)
        try:
            while True:
                datagram, address = server.recvfrom(2048)
                sender = f"{address[0]}:{address[1]}"
                try:
                    message = parse_alert(datagram)
                except AlertValidationError as exc:
                    print(f"Ignored invalid packet from {sender}: {exc}", file=sys.stderr)
                    continue
                if deduplicator.is_duplicate(address[0], message):
                    print(f"Ignored duplicate sequence {message.sequence} from {sender}")
                    continue

                print(message.summary(), flush=True)
                raw_payload = json.loads(datagram)
                if args.log is not None:
                    try:
                        _append_log(args.log, raw_payload, sender)
                    except OSError as exc:
                        print(f"Could not write alert log: {exc}", file=sys.stderr)
                if args.notify:
                    try:
                        _desktop_notification(message)
                    except (OSError, subprocess.SubprocessError) as exc:
                        print(f"Desktop notification failed: {exc}", file=sys.stderr)
                if args.webhook_url:
                    try:
                        _forward_webhook(args.webhook_url, datagram, args.webhook_timeout)
                    except (OSError, urllib.error.URLError) as exc:
                        print(f"Webhook delivery failed (receiver continues): {exc}", file=sys.stderr)
        except KeyboardInterrupt:
            print("\nReceiver stopped.")
    return 0


def main(argv: list[str] | None = None) -> int:
    return run_receiver(build_argument_parser().parse_args(argv))


if __name__ == "__main__":
    raise SystemExit(main())
