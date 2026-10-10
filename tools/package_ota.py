#!/usr/bin/env python3
"""Validate and package an ESP32-S3 application image, without device access."""
import hashlib
import json
from pathlib import Path
import struct


class ImageError(ValueError):
    pass


def inspect_application(data):
    if len(data) < 32 or data[0] != 0xE9:
        raise ImageError("Expected an application image at offset zero, not a merged image.")
    segments = data[1]
    if not 1 <= segments <= 16 or struct.unpack_from("<H", data, 12)[0] != 9:
        raise ImageError("Image must target ESP32-S3 with valid segments.")
    if data[23] not in (0, 1):
        raise ImageError("Invalid hash flag.")
    cursor = 24
    checksum = 0xEF
    for index in range(segments):
        if cursor + 8 > len(data):
            raise ImageError("Truncated segment header.")
        _, length = struct.unpack_from("<II", data, cursor)
        cursor += 8
        if cursor + length > len(data):
            raise ImageError("Truncated image segment.")
        payload = data[cursor:cursor + length]
        if index == 0 and (length < 256 or struct.unpack_from("<I", payload)[0] != 0xABCD5432):
            raise ImageError("Missing application descriptor; refusing a bootloader image.")
        for byte in payload:
            checksum ^= byte
        cursor += length
    checksum_offset = ((cursor + 1 + 15) // 16) * 16 - 1
    if checksum_offset >= len(data) or data[checksum_offset] != checksum:
        raise ImageError("Application checksum mismatch.")
    payload_end = checksum_offset + 1
    if data[23]:
        if len(data) != payload_end + 32:
            raise ImageError("Truncated hash or unsupported trailing image data.")
        if hashlib.sha256(data[:payload_end]).digest() != data[payload_end:]:
            raise ImageError("Application embedded SHA-256 mismatch.")
    elif len(data) != payload_end:
        raise ImageError("Unexpected trailing image data.")
    return {"chip": "esp32-s3", "segments": segments, "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(), "application_descriptor": True,
            "embedded_sha256": bool(data[23])}


def package_application(binary, destination, contract, metadata, diagnostic=False):
    data = Path(binary).read_bytes()
    details = inspect_application(data)
    if contract.get("installer") != "M5Launcher" or contract.get("target") != "esp32-s3":
        raise ImageError("Unsupported installer or target contract.")
    limit = contract.get("installed_app_slot_bytes")
    verified = (contract.get("user_confirmed_installer") is True and
                contract.get("installed_layout_verified") is True and
                bool(contract.get("installed_version")) and
                isinstance(limit, int) and not isinstance(limit, bool) and limit > 0)
    if limit is not None and (not isinstance(limit, int) or isinstance(limit, bool) or limit <= 0):
        raise ImageError("Invalid app slot limit.")
    if limit and len(data) > limit:
        raise ImageError(f"Application size {len(data)} exceeds installed slot limit {limit}.")
    publication_authorized = metadata.get("publication_authorized") is True and metadata.get("milestone")=="release"
    if not diagnostic and not verified and not publication_authorized:
        raise ImageError("Installed launcher layout is unverified; only a diagnostic candidate can be packaged.")
    stage = metadata.get("milestone", "M1" if diagnostic else "release")
    if stage not in (("M1", "M2", "M3", "M4", "M5", "M6", "M7", "M8") if diagnostic else ("release",)):
        raise ImageError("Unsupported package milestone.")
    mesh_enabled = stage in ("M2", "M3", "M4", "M5", "M6", "M7", "M8") or (stage=="release" and metadata.get("mesh_runtime_enabled") is True)
    if "mesh_runtime_enabled" in metadata and metadata["mesh_runtime_enabled"] is not mesh_enabled:
        raise ImageError("Unsupported package milestone.")
    suffix = "-candidate" if diagnostic and stage in ("M2", "M3", "M4", "M5", "M6", "M7", "M8") else "-diagnostic" if diagnostic else ""
    name = f"cardputer-adv-{metadata['version']}{suffix}-ota.bin"
    out = Path(destination)
    out.mkdir(parents=True, exist_ok=True)
    image_path = out / name
    image_path.write_bytes(data)
    manifest = {"format": "application-only", "stage": stage,
                "filename": name, "image": details, "build": metadata,
                "ota_contract": contract, "installation_compatibility_verified": verified,
                "hardware_validated": False, "mesh_runtime_enabled": mesh_enabled}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (out / "SHA256SUMS.txt").write_text(f"{details['sha256']}  {name}\n")
    return manifest
