import json
import random
import unittest
from contextlib import redirect_stdout
from io import StringIO

from simulation.main import VitalSimulator
from simulation.patient import PatientModel
from simulation.scenarios import SCENARIOS
from simulation.uart import UARTTransport, encode_frame


class FakeTransport:
    port = None
    baudrate = 115200

    def __init__(self):
        self.frames = []
        self.closed = False

    def connect(self):
        return False

    def send(self, payload):
        line = encode_frame(payload).decode("utf-8")
        self.frames.append(line)
        return "Mock", line

    def close(self):
        self.closed = True


class PatientModelTests(unittest.TestCase):
    def test_every_scenario_starts_with_meaningful_values(self):
        for index, (name, scenario) in enumerate(SCENARIOS.items()):
            with self.subTest(scenario=name):
                patient = PatientModel(name, rng=random.Random(index))
                sample = patient.sample(0.02)
                self.assertLessEqual(scenario.heart_rate[0] - 1, sample["hr"])
                self.assertLessEqual(sample["hr"], scenario.heart_rate[1] + 1)
                self.assertLessEqual(scenario.spo2[0] - 1, sample["spo2"])
                self.assertLessEqual(sample["spo2"], scenario.spo2[1] + 1)
                self.assertLessEqual(scenario.temperature[0] - 0.1, sample["temp"])
                self.assertLessEqual(sample["temp"], scenario.temperature[1] + 0.1)

    def test_scenario_change_is_continuous(self):
        patient = PatientModel("Normal", rng=random.Random(4))
        before = patient.current_hr
        patient.set_scenario("Tachycardia")
        patient.sample(0.02)
        self.assertLess(abs(patient.current_hr - before), 0.3)

    def test_scenario_change_converges(self):
        patient = PatientModel("Normal", rng=random.Random(8))
        patient.set_scenario("Tachycardia")
        for _ in range(500):
            patient.sample(0.02)
        low, high = SCENARIOS["Tachycardia"].heart_rate
        self.assertLessEqual(low - 1, patient.current_hr)
        self.assertLessEqual(patient.current_hr, high + 1)

    def test_ecg_contains_qrs_peaks(self):
        patient = PatientModel("Normal", rng=random.Random(3))
        values = [patient.sample(0.01)["ecg"] for _ in range(300)]
        self.assertGreater(max(values), 0.8)
        self.assertLess(min(values), -0.05)

    def test_sample_schema(self):
        sample = PatientModel("Normal", rng=random.Random(1)).sample(0.02)
        self.assertEqual({"scenario", "hr", "spo2", "temp", "ecg"}, set(sample))
        self.assertIsInstance(sample["scenario"], str)
        self.assertIsInstance(sample["hr"], int)
        self.assertIsInstance(sample["spo2"], int)
        self.assertIsInstance(sample["temp"], float)
        self.assertIsInstance(sample["ecg"], float)


class UARTTests(unittest.TestCase):
    def test_frame_is_compact_newline_delimited_json(self):
        payload = {"scenario": "Normal", "hr": 72, "spo2": 98, "temp": 36.8, "ecg": 0.1}
        frame = encode_frame(payload)
        self.assertTrue(frame.endswith(b"\n"))
        self.assertNotIn(b" ", frame)
        self.assertEqual(payload, json.loads(frame))

    def test_pyserial_loopback_receives_the_exact_frame(self):
        payload = {"scenario": "Normal", "hr": 70, "spo2": 97, "temp": 36.7, "ecg": -0.02}
        transport = UARTTransport("loop://")
        self.assertTrue(transport.connect())
        try:
            mode, line = transport.send(payload)
            received = transport.serial.readline()
            self.assertEqual("Sent", mode)
            self.assertEqual(line.encode("utf-8"), received)
        finally:
            transport.close()

    def test_simulator_sends_requested_number_of_frames(self):
        transport = FakeTransport()
        simulator = VitalSimulator(
            transport=transport,
            sample_interval=0.001,
            console_interval=10,
            seed=2,
        )
        with redirect_stdout(StringIO()):
            count = simulator.run(samples=3)
        self.assertEqual(3, count)
        self.assertEqual(3, len(transport.frames))
        self.assertTrue(transport.closed)


if __name__ == "__main__":
    unittest.main()
