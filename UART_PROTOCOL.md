# UART protocol contract

This document is the integration contract between the Python simulator and the
STM32 firmware.

## Serial settings

- Baud rate: `115200`
- Data bits: `8`
- Parity: none
- Stop bits: `1`
- Flow control: none
- Encoding: UTF-8 (the current payload is ASCII-compatible)

## Framing

Each frame is one compact JSON object followed by a single line-feed byte
(`\n`, hexadecimal `0A`). The receiver must buffer bytes until `\n`, reject an
oversized or invalid frame, and then continue with the next line.

Example:

```json
{"scenario":"Normal","hr":74,"spo2":98,"temp":36.8,"ecg":0.021}
```

## Fields

| Field | JSON type | Meaning | Expected simulator range |
| --- | --- | --- | --- |
| `scenario` | string | Active patient condition | One of the five names below |
| `hr` | integer | Heart rate in BPM | 42–140 in current scenarios |
| `spo2` | integer | Blood oxygen saturation in percent | 84–99 |
| `temp` | number | Body temperature in degrees Celsius | 36.4–39.5 |
| `ecg` | number | Normalized instantaneous ECG amplitude | Approximately -0.3–1.3 |

Valid scenarios are `Normal`, `Tachycardia`, `Bradycardia`, `Low_SpO2`, and
`Abnormal_Temp`.

The default rate is 50 frames per second. A typical frame is under 80 bytes, so
it remains comfortably below the capacity of 115200 baud UART.

## Receiver requirements

- Use at least a 128-byte line buffer.
- Update displayed values only after a complete, valid frame is parsed.
- Ignore unknown fields so the protocol can be extended later.
- Treat a missing required field, invalid JSON, or an overlong line as a bad
  frame; discard it without blocking subsequent frames.
- Consider data stale if no valid frame arrives for two seconds.
