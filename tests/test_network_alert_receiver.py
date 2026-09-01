import json
import unittest

from network_alert.receiver import (
    AlertValidationError,
    SequenceDeduplicator,
    parse_alert,
)


def payload(**overrides):
    value = {
        "version": 1,
        "type": "vital_alert",
        "event": "triggered",
        "sequence": 1,
        "uptime_ms": 1234,
        "severity": "warning",
        "scenario": "Low_SpO2",
        "heart_rate": 78,
        "spo2": 89,
        "temperature_c": 36.9,
        "reasons": ["low_spo2"],
    }
    value.update(overrides)
    return json.dumps(value).encode()


class AlertReceiverTests(unittest.TestCase):
    def test_valid_alert_is_parsed(self):
        message = parse_alert(payload())
        self.assertEqual(message.scenario, "Low_SpO2")
        self.assertEqual(message.reasons, ("low_spo2",))
        self.assertTrue(message.is_alarm)

    def test_valid_recovery_is_parsed(self):
        message = parse_alert(
            payload(event="recovered", severity="normal", reasons=[], sequence=2)
        )
        self.assertFalse(message.is_alarm)

    def test_invalid_json_is_rejected(self):
        with self.assertRaises(AlertValidationError):
            parse_alert(b"not-json")

    def test_active_alert_needs_reason(self):
        with self.assertRaises(AlertValidationError):
            parse_alert(payload(reasons=[]))

    def test_out_of_range_spo2_is_rejected(self):
        with self.assertRaises(AlertValidationError):
            parse_alert(payload(spo2=101))

    def test_duplicate_sequence_is_ignored_per_sender(self):
        deduplicator = SequenceDeduplicator()
        first = parse_alert(payload(sequence=7, uptime_ms=5000))
        duplicate = parse_alert(payload(sequence=7, uptime_ms=5100))
        next_message = parse_alert(payload(sequence=8, uptime_ms=5200))
        self.assertFalse(deduplicator.is_duplicate("192.168.7.2", first))
        self.assertTrue(deduplicator.is_duplicate("192.168.7.2", duplicate))
        self.assertFalse(deduplicator.is_duplicate("192.168.7.2", next_message))


if __name__ == "__main__":
    unittest.main()
