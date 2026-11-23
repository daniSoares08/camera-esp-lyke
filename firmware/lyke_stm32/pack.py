"""Build the initial DFU image and the STM32 image for the ESP portal."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib


def verify(data, origin, limit):
    size = len(data)
    if not 512 <= size <= limit:
        raise SystemExit(f"Invalid image size at {origin:#x}: {size}")
    stack, reset = struct.unpack_from("<II", data)
    if stack != 0x200057F0 or not (reset & 1 and origin + 8 <= reset < origin + size):
        raise SystemExit(f"Invalid vectors at {origin:#x}")


def main():
    out = Path(sys.argv[1])
    loader = (out / "boot.bin").read_bytes()
    app = (out / "main.bin").read_bytes()
    verify(loader, 0x08000000, 0x2000)
    verify(app, 0x08002000, 0xE000)
    crc = zlib.crc32(app)
    artifacts = {
        "lyke-stm32-dfu.bin": loader.ljust(0x2000, b"\xff") + app,
        "lyke-stm32-ota.bin": struct.pack("<8sII", b"LYKEG431", len(app), crc) + app,
    }
    manifest = {
        "target": "STM32G431CBU6",
        "revision": sys.argv[2],
        "boot_origin": "0x08000000",
        "app_origin": "0x08002000",
        "stage_origin": "0x08010000",
        "keymap_origin": "0x0801e800",
        "app_bytes": len(app),
        "app_crc32": f"{crc:08x}",
        "artifacts": {},
    }
    for name, data in artifacts.items():
        (out / name).write_bytes(data)
        manifest["artifacts"][name] = {
            "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()
        }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"App: {len(app)} / 57344 bytes. DFU and OTA: {out}")


if __name__ == "__main__":
    main()
