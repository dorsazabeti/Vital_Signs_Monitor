"""Command-line entry point and orchestration for the vital-sign simulator."""

import argparse
import random
import time
from typing import Optional, Sequence

from .patient import PatientModel
from .scenarios import SCENARIOS
from .uart import UARTTransport, available_ports


class VitalSimulator:
    """Coordinate patient sampling, UART framing, and real-time pacing."""

    def __init__(
        self,
        port: Optional[str] = None,
        baudrate: int = 115200,
        scenario: str = "Normal",
        sample_interval: float = 0.02,
        target_update_interval: float = 4.0,
        console_interval: float = 0.5,
        seed: Optional[int] = None,
        transport: Optional[UARTTransport] = None,
    ) -> None:
        if sample_interval <= 0:
            raise ValueError("sample_interval must be positive")
        if console_interval < 0:
            raise ValueError("console_interval cannot be negative")

        self.sample_interval = sample_interval
        self.console_interval = console_interval
        self.patient = PatientModel(
            scenario=scenario,
            target_update_interval=target_update_interval,
            rng=random.Random(seed),
        )
        self.transport = transport or UARTTransport(port=port, baudrate=baudrate)

    @property
    def scenario(self) -> str:
        return self.patient.scenario_name

    def set_scenario(self, scenario: str) -> None:
        self.patient.set_scenario(scenario)
        print(f"[*] Scenario changed to: {scenario}")

    def get_data(self, dt: Optional[float] = None):
        interval = self.sample_interval if dt is None else dt
        return self.patient.sample(interval)

    def run(
        self,
        duration: Optional[float] = None,
        samples: Optional[int] = None,
    ) -> int:
        """Run in real time and return the number of transmitted samples."""

        if duration is not None and duration <= 0:
            raise ValueError("duration must be positive")
        if samples is not None and samples <= 0:
            raise ValueError("samples must be positive")

        connected = self.transport.connect()
        if connected:
            print(f"[*] Connected to {self.transport.port} @ {self.transport.baudrate} baud")
        else:
            if self.transport.port:
                print(
                    f"[!] Could not open {self.transport.port}; running in MOCK mode "
                    "and retrying automatically."
                )
            else:
                print("[*] No UART port selected; running in MOCK mode.")

        print(f"[*] Scenario: {self.scenario}")
        print(f"[*] Sample rate: {1.0 / self.sample_interval:.1f} Hz")
        print("[*] Press Ctrl+C to stop.")

        started = time.monotonic()
        last_tick = started
        next_tick = started
        last_console = started - self.console_interval
        sent_count = 0

        try:
            while True:
                now = time.monotonic()
                if duration is not None and now - started >= duration:
                    break
                if samples is not None and sent_count >= samples:
                    break

                dt = now - last_tick
                last_tick = now
                payload = self.get_data(max(dt, 0.001))
                mode, json_line = self.transport.send(payload)
                sent_count += 1

                if self.console_interval == 0 or now - last_console >= self.console_interval:
                    print(f"{mode}: {json_line.strip()}")
                    last_console = now

                next_tick += self.sample_interval
                sleep_time = next_tick - time.monotonic()
                if sleep_time > 0:
                    time.sleep(sleep_time)
                else:
                    next_tick = time.monotonic()
        except KeyboardInterrupt:
            print("\n[!] Stopped by user.")
        finally:
            self.transport.close()

        print(f"[*] Finished after {sent_count} samples.")
        return sent_count


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Generate meaningful vital signs and send newline-delimited JSON over UART."
    )
    parser.add_argument("--port", help="Serial port; omit to run safely in mock mode")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--scenario", choices=tuple(SCENARIOS), default="Normal")
    parser.add_argument(
        "--sample-rate",
        type=float,
        default=50.0,
        help="Samples per second (default: 50)",
    )
    parser.add_argument(
        "--console-interval",
        type=float,
        default=0.5,
        help="Seconds between console previews; use 0 for every frame",
    )
    parser.add_argument("--duration", type=float, help="Stop after this many seconds")
    parser.add_argument("--samples", type=int, help="Stop after this many frames")
    parser.add_argument("--seed", type=int, help="Seed for a reproducible demo")
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List detected serial ports and exit",
    )
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.list_ports:
        ports = available_ports()
        if ports:
            print("\n".join(ports))
        else:
            print("No serial ports detected.")
        return 0

    if args.sample_rate <= 0:
        parser.error("--sample-rate must be positive")
    if args.baudrate <= 0:
        parser.error("--baudrate must be positive")
    if args.console_interval < 0:
        parser.error("--console-interval cannot be negative")
    if args.duration is not None and args.duration <= 0:
        parser.error("--duration must be positive")
    if args.samples is not None and args.samples <= 0:
        parser.error("--samples must be positive")

    simulator = VitalSimulator(
        port=args.port,
        baudrate=args.baudrate,
        scenario=args.scenario,
        sample_interval=1.0 / args.sample_rate,
        console_interval=args.console_interval,
        seed=args.seed,
    )
    simulator.run(duration=args.duration, samples=args.samples)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
