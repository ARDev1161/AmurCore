"""True old/new protobuf compatibility using two independent Python processes.

Generated old (dev) and new (feature) message modules cannot coexist in one
default protobuf descriptor pool. Subprocesses use distinct PYTHONPATHs to
verify both directions across the actual serialized wire.
"""
import base64
import os
from pathlib import Path
import subprocess
import sys
import unittest

import robot_pb2

OLD_PROTO = Path(os.environ.get("LEGACY_PROTO_PY", "legacy-generated")).resolve()


def old_wire(mode, data=b""):
    source = r"""
import base64, sys
import robot_pb2
mode = sys.argv[1]
if mode == 'write':
    m = robot_pb2.Sensors()
    m.batteryState.percentage = 0.625
    m.CPUTemperature = 41
    print(base64.b64encode(m.SerializeToString()).decode('ascii'))
elif mode == 'parse_new':
    m = robot_pb2.Sensors()
    m.ParseFromString(base64.b64decode(sys.argv[2]))
    assert m.batteryState.percentage == 0.75
    assert m.CPUTemperature == 44
    assert 'robot_info' not in m.DESCRIPTOR.fields_by_name
    print('legacy parser accepted the new payload')
else:
    raise ValueError(mode)
"""
    env = os.environ.copy()
    env["PYTHONPATH"] = str(OLD_PROTO)
    result = subprocess.run([sys.executable, "-c", source, mode,
                             base64.b64encode(data).decode("ascii")],
                            env=env, capture_output=True, text=True, timeout=8,
                            check=True)
    return result.stdout.strip()


class OldNewInterop(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (OLD_PROTO / "robot_pb2.py").is_file():
            raise RuntimeError("Missing legacy-generated/robot_pb2.py; generate dev protos first")

    def test_new_receiver_reads_unmodified_legacy_telemetry(self):
        legacy_wire = base64.b64decode(old_wire("write"))
        current = robot_pb2.Sensors.FromString(legacy_wire)
        self.assertFalse(current.HasField("robot_info"))
        self.assertAlmostEqual(current.batteryState.percentage, 0.625)
        self.assertEqual(current.CPUTemperature, 41)

    def test_legacy_receiver_ignores_new_robot_descriptor(self):
        current = robot_pb2.Sensors()
        current.batteryState.percentage = 0.75
        current.CPUTemperature = 44
        current.robot_info.robot_id = "robot-with-v2"
        current.robot_info.api_major = 2
        current.robot_info.capabilities.add(id="manual.velocity",
                                            velocity_axes=["linear.x", "angular.z"])
        result = old_wire("parse_new", current.SerializeToString())
        self.assertIn("accepted", result)


if __name__ == "__main__":
    unittest.main()
