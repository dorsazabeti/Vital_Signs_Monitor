# Vital Signs Monitor — PC Simulator

This repository contains the PC-side simulator and the in-progress STM32 code
for IoT Laboratory Project 1. The simulator generates Heart Rate, SpO2, body
temperature, and a continuous ECG waveform for meaningful patient scenarios.
It sends compact newline-delimited JSON to the STM32 through a USB-to-Serial
UART bridge.

## Implemented simulator features

- Five scenarios: `Normal`, `Tachycardia`, `Bradycardia`, `Low_SpO2`, and
  `Abnormal_Temp`
- Smooth changes within a scenario and continuous transitions between scenarios
- ECG waveform with P, Q, R, S, and T components, baseline drift, and noise
- JSON framing at a configurable sample rate (50 Hz by default)
- Real UART output or safe mock mode when no port is selected
- Reproducible output through an optional random seed
- Automated tests for scenario ranges, transitions, ECG, framing, and a
  PySerial loopback transmission

## Setup

Python 3.8 or newer is required.


```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

## Usage

List detected serial ports:

```bash
python3 -m simulation --list-ports
```

Run without hardware in mock mode:

```bash
python3 -m simulation --scenario Normal --duration 5
```

Send Tachycardia data to a connected UART adapter:

```bash
python3 -m simulation \
  --port /dev/cu.usbserial-XXXX \
  --scenario Tachycardia \
  --sample-rate 50
```

The old launcher also remains valid:

```bash
python3 main.py --scenario Low_SpO2 --samples 100
```

Run `python3 -m simulation --help` for all options.

## UART integration contract

The default link is `115200 8N1`. Each transmission is one UTF-8 JSON object
terminated by `\n`:

```json
{"scenario":"Normal","hr":74,"spo2":98,"temp":36.8,"ecg":0.021}
```

Field types, framing rules, error handling, and STM32 receiver requirements are
specified in [UART_PROTOCOL.md](UART_PROTOCOL.md).

## Tests

Tests use Python's standard library and do not require a board:

```bash
python3 -m unittest discover -s tests -v
```

Hardware UART reception, STM32 JSON parsing, GUI updates, and Ethernet alerts
must still be verified as part of end-to-end integration.

## Team ownership from the proposal

- **Dorsa Zabeti:** Python simulator, vital-sign generation, JSON framing,
  scenarios, and simulator documentation
- **Arad Izadidoost:** STM32 bring-up, UART reception, and embedded integration
- **Yasaman Farrokhi:** graphical assets/UI and networking/alert work
- **Shared:** end-to-end testing, final documentation, and demo
