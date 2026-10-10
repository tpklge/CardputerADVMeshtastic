import hashlib
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from package_ota import ImageError, inspect_application, package_application


def application(chip=9, descriptor=0xABCD5432):
    header = bytearray(24)
    header[0:2] = bytes([0xE9, 1])
    struct.pack_into("<H", header, 12, chip)
    header[23] = 1
    payload = bytearray(256)
    struct.pack_into("<I", payload, 0, descriptor)
    data = header + struct.pack("<II", 0x3C000020, len(payload)) + payload
    checksum = 0xEF
    for byte in payload:
        checksum ^= byte
    while len(data) % 16 != 15:
        data.append(0)
    data.append(checksum)
    return bytes(data + hashlib.sha256(data).digest())


class OtaTests(unittest.TestCase):
    def test_m8_candidate_preserves_validation_gate(self):
        contract = {"installer": "M5Launcher", "target": "esp32-s3"}
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);binary=root/"firmware.bin";binary.write_bytes(application())
            m=package_application(binary,root/"m8",contract,{"version":"0.8.0-m8","milestone":"M8","mesh_runtime_enabled":True,"application_schema":1},diagnostic=True)
            self.assertEqual(m["stage"],"M8")
            self.assertFalse(m["hardware_validated"])
            self.assertFalse(m["installation_compatibility_verified"])
            self.assertTrue(m["mesh_runtime_enabled"])
            self.assertIn("-candidate-ota.bin",m["filename"])

    def test_authorized_release_keeps_unverified_installation_explicit(self):
        contract={"installer":"M5Launcher","target":"esp32-s3","user_confirmed_installer":True}
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);binary=root/"firmware.bin";binary.write_bytes(application())
            meta={"version":"1.0.0","milestone":"release","publication_authorized":True,"mesh_runtime_enabled":True}
            m=package_application(binary,root/"release",contract,meta)
            self.assertEqual(m["filename"],"cardputer-adv-1.0.0-ota.bin")
            self.assertFalse(m["hardware_validated"])
            self.assertFalse(m["installation_compatibility_verified"])
            self.assertTrue(m["mesh_runtime_enabled"])
            contract["installed_app_slot_bytes"]=100
            with self.assertRaises(ImageError):package_application(binary,root/"oversize",contract,meta)

    def test_valid_application(self):
        self.assertEqual(inspect_application(application())["chip"], "esp32-s3")

    def test_wrong_chip_bootloader_and_merged(self):
        for data in (application(chip=0), application(descriptor=0), b"\xff" * 4096 + application()):
            with self.subTest(data=data[:32]):
                with self.assertRaises(ImageError):
                    inspect_application(data)

    def test_corrupt_or_truncated_image(self):
        image = application()
        corrupted = bytearray(image)
        corrupted[100] ^= 1
        for data in (bytes(corrupted), image[:-1], image[:30], image + b"garbage"):
            with self.assertRaises(ImageError):
                inspect_application(data)

    def test_contract_and_size_gate_before_output(self):
        contract = {"installer": "M5Launcher", "target": "esp32-s3"}
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            binary = root / "firmware.bin"
            binary.write_bytes(application())
            with self.assertRaises(ImageError):
                package_application(binary, root / "release", contract, {"version": "test"})
            self.assertFalse((root / "release").exists())
            manifest = package_application(binary, root / "candidate", contract, {"version": "test"}, diagnostic=True)
            self.assertFalse(manifest["installation_compatibility_verified"])
            contract.update(user_confirmed_installer=True, installed_version="confirmed",
                            installed_layout_verified=True, installed_app_slot_bytes=100)
            with self.assertRaises(ImageError):
                package_application(binary, root / "oversize", contract, {"version": "test"}, diagnostic=True)
            self.assertFalse((root / "oversize").exists())
            contract["installed_app_slot_bytes"] = 4096
            manifest = package_application(binary, root / "fitting", contract, {"version": "test"})
            self.assertTrue(manifest["installation_compatibility_verified"])

    def test_m2_candidate_reports_mesh_runtime(self):
        contract = {"installer": "M5Launcher", "target": "esp32-s3"}
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            binary = root / "firmware.bin"
            binary.write_bytes(application())
            meta = {"version": "0.2.0-m2", "milestone": "M2", "mesh_runtime_enabled": True}
            m = package_application(binary, root / "m2", contract, meta, diagnostic=True)
            self.assertEqual(m["stage"], "M2")
            self.assertTrue(m["mesh_runtime_enabled"])
            self.assertFalse(m["hardware_validated"])
            self.assertFalse(m["installation_compatibility_verified"])
            self.assertIn("-candidate-ota.bin", m["filename"])
            meta["mesh_runtime_enabled"] = False
            with self.assertRaises(ImageError):
                package_application(binary, root / "bad", contract, meta, diagnostic=True)
            self.assertFalse((root / "bad").exists())

    def test_m6_candidate_reports_schema_and_runtime(self):
        contract = {"installer": "M5Launcher", "target": "esp32-s3"}
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp);binary = root/"firmware.bin";binary.write_bytes(application())
            m=package_application(binary,root/"m6",contract,{"version":"0.6.0-m6","milestone":"M6","mesh_runtime_enabled":True,"application_schema":1},diagnostic=True)
            self.assertEqual(m["stage"],"M6");self.assertTrue(m["mesh_runtime_enabled"]);self.assertFalse(m["hardware_validated"])

    def test_m5_candidate_reports_schema_and_runtime(self):
        contract = {"installer": "M5Launcher", "target": "esp32-s3"}
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            binary = root / "firmware.bin"
            binary.write_bytes(application())
            metadata = {"version": "0.5.0-m5", "milestone": "M5", "mesh_runtime_enabled": True, "application_schema": 1}
            manifest = package_application(binary, root / "m5", contract, metadata, diagnostic=True)
            self.assertEqual(manifest["stage"], "M5")
            self.assertEqual(manifest["build"]["application_schema"], 1)
            self.assertTrue(manifest["mesh_runtime_enabled"])
            self.assertFalse(manifest["hardware_validated"])
            self.assertIn("0.5.0-m5-candidate-ota.bin", manifest["filename"])

    def test_m7_candidate_is_unvalidated_application_only(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            binary = root / "firmware.bin"
            binary.write_bytes(application())
            manifest = package_application(binary, root / "m7", {"installer":"M5Launcher", "target":"esp32-s3"}, {"version":"0.7.0-m7", "milestone":"M7", "application_schema":1}, diagnostic=True)
            self.assertTrue(manifest["mesh_runtime_enabled"])
            self.assertFalse(manifest["hardware_validated"])
            self.assertFalse(manifest["installation_compatibility_verified"])
            self.assertIn("0.7.0-m7-candidate-ota.bin", manifest["filename"])

    def test_m4_candidate_reports_schema_and_runtime(self):
        contract = {"installer": "M5Launcher", "target": "esp32-s3"}
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            binary = root / "firmware.bin"
            binary.write_bytes(application())
            metadata = {"version": "0.4.0-m4", "milestone": "M4", "mesh_runtime_enabled": True, "application_schema": 1}
            manifest = package_application(binary, root / "m4", contract, metadata, diagnostic=True)
            self.assertEqual(manifest["stage"], "M4")
            self.assertEqual(manifest["build"]["application_schema"], 1)
            self.assertTrue(manifest["mesh_runtime_enabled"])
            self.assertFalse(manifest["hardware_validated"])
            self.assertIn("0.4.0-m4-candidate-ota.bin", manifest["filename"])

    def test_m3_candidate_reports_schema_and_runtime(self):
        contract = {"installer": "M5Launcher", "target": "esp32-s3"}
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            binary = root / "firmware.bin"
            binary.write_bytes(application())
            metadata = {"version": "0.3.0-m3", "milestone": "M3", "mesh_runtime_enabled": True, "application_schema": 1}
            manifest = package_application(binary, root / "m3", contract, metadata, diagnostic=True)
            self.assertTrue(manifest["mesh_runtime_enabled"])
            self.assertEqual(manifest["build"]["application_schema"], 1)
            self.assertFalse(manifest["hardware_validated"])
            self.assertIn("candidate-ota.bin", manifest["filename"])


if __name__ == "__main__":
    unittest.main()
