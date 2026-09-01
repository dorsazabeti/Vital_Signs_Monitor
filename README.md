# Vital Signs Monitor — PC Simulator

This repository contains the complete software path for IoT Laboratory Project
1: the PC-side patient simulator, STM32F746G-DISCO firmware, LVGL dashboard,
and Ethernet alert receiver. The simulator generates Heart Rate, SpO2, body
temperature, and a continuous ECG waveform for meaningful patient scenarios,
then sends compact newline-delimited JSON through the board's USB Virtual COM
Port.

## Implemented simulator features

- Five scenarios: `Normal`, `Tachycardia`, `Bradycardia`, `Low_SpO2`, and
  `Abnormal_Temp`
- Smooth changes within a scenario and continuous transitions between scenarios
- ECG waveform with P, Q, R, S, and T components, baseline drift, and noise
- JSON framing at a configurable sample rate (50 Hz by default)
- Real UART output or safe mock mode when no port is selected
- Automatic serial reconnection after the board or port is disconnected
- Reproducible output through an optional random seed
- Automated tests for scenario ranges, transitions, ECG, framing, and a
  PySerial loopback transmission

## Implemented firmware features

- Interrupt-driven USART1 reception with an eight-frame queue, overflow/error
  counters, and automatic receive recovery
- Validated fixed-point parsing for all required fields, including negative ECG
  samples
- LVGL dashboard with live HR, SpO2, temperature, scenario, animated heart,
  moving gauge/thermometer indicators, and a scrolling ECG chart
- Shared normal/warning/critical thresholds for both the UI and network alerts
- A visible `NO DATA` state when no valid UART frame arrives for two seconds
- Static-IP Ethernet with ARP, ICMP echo reply, and UDP/JSON alerts
- Debounced abnormal-state detection, 30-second reminder rate limiting, and a
  recovery event

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

## Ethernet alerts

The STM32 firmware now includes a small static-IP Ethernet alert path (ARP,
IPv4, UDP, and ping reply) that does not require LwIP. Abnormal values are
debounced and rate-limited before the board sends a validated JSON alert to the
computer. The receiver can log alerts, show macOS desktop notifications, and
optionally forward them to an HTTP webhook.

Default direct-cable addresses are `192.168.7.1` for the computer and
`192.168.7.2` for the board. Start the receiver with:

```bash
python3 -m network_alert --log alerts.jsonl
```

Configuration, payload fields, cable setup, and the hardware test checklist are
documented in
[CubeIDEProject/docs/NETWORK_ALERT.md](CubeIDEProject/docs/NETWORK_ALERT.md).

## Tests

Run the complete host-side test suite (Python plus the two independent C
modules) with:

```bash
./scripts/run_tests.sh
```

The Python portion can also be run separately with
`python3 -m unittest discover -s tests -v`.

## Firmware build

Install GNU Tools for STM32/Arm and CMake, then run from the repository root:

```bash
cmake -S CubeIDEProject -B CubeIDEProject/build \
  -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi.cmake
cmake --build CubeIDEProject/build -j4
```

If `arm-none-eabi-gcc` is not on `PATH`, also pass
`-DSTM32_GNU_TOOLS_PATH=/absolute/path/to/toolchain/bin`.

The software tests are complete. Flashing the generated firmware and recording
the final LCD, UART, Ethernet Link/Ping/UDP results still require the physical
board and cable; see the checklist in
[CubeIDEProject/docs/PROJECT_CONTEXT.md](CubeIDEProject/docs/PROJECT_CONTEXT.md).

## Team ownership from the proposal

- **Dorsa Zabeti:** Python simulator, vital-sign generation, JSON framing,
  scenarios, and simulator documentation
- **Arad Izadidoost:** STM32 bring-up, UART reception, and embedded integration
- **Yasaman Farrokhi:** graphical assets/UI and networking/alert work
- **Shared:** end-to-end testing, final documentation, and demo
