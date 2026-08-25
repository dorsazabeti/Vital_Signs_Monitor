"""Backward-compatible launcher for the simulator package."""

from simulation.main import main


if __name__ == "__main__":
    raise SystemExit(main())
