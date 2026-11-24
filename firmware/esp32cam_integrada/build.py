"""Build classic ESP32 firmware. Never flashes or embeds cloud credentials."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
NAME = 'esp32cam_integrada'
SLOT = 0x180000


def run(*args):
    subprocess.run([str(x) for x in args], check=True, cwd=ROOT)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--revision', required=True)
    p.add_argument('--optimization', choices=['baseline', 'lto'], default='lto')
    p.add_argument('--profile', choices=['bench', 'lyke'], default='bench')
    p.add_argument('--ap-password-file', type=Path, default=ROOT/'.private/ap-password.txt',
                   help='Local-only WPA2 password; never published with source')
    args = p.parse_args()
    if not args.revision.isascii() or len(args.revision) > 31 or not all(c.isalnum() or c in '-_.' for c in args.revision):
        p.error('revision must be 1-31 ASCII letters/numbers/dashes')
    sdk = Path(os.environ['LOCALAPPDATA'])/'Arduino15/packages/esp32/hardware/esp32/3.3.6'
    gcc = Path(os.environ['LOCALAPPDATA'])/'Arduino15/packages/esp32/tools/esp-x32/2511/bin'
    workspace = ROOT/'.build'/('integrada_'+args.optimization+('_lyke' if args.profile == 'lyke' else ''))
    stage = workspace/NAME
    build = workspace/'build'
    out = HERE/'bin'/args.revision
    for folder in (stage, build, out):
        folder.mkdir(parents=True, exist_ok=True)
    for source in (HERE/NAME).iterdir():
        if source.suffix in ('.ino', '.h', '.csv'):
            shutil.copy2(source, stage/source.name)
    shared = ROOT/'firmware/calculadora/common'
    for name in ('calculator_core.h', 'font5x7.h', 'FONT-LICENSE.txt'):
        shutil.copy2(shared/name, stage/name)
    if args.profile == 'lyke':
        for source in (HERE/'lyke').iterdir():
            if source.suffix in ('.ino', '.h'):
                shutil.copy2(source, stage/source.name)
        for name in ('api_contract.h', 'vision_body.h'):
            shutil.copy2(shared/name, stage/name)
        image_format = (ROOT/'firmware/lyke_stm32/image_format.h').read_text()
        shutil.copy2(ROOT/'firmware/lyke_stm32/answer_frame.h',stage/'answer_frame.h')
        (stage/'image_format.h').write_text(image_format.replace('"vendor/puff.h"', '"puff.h"'))
        for name in ('puff.c', 'puff.h'):
            shutil.copy2(ROOT/'firmware/lyke_stm32/vendor'/name, stage/name)
    source = (ROOT/'firmware/camera_usb/serial_camera_usb.ino').read_text(encoding='utf-8-sig')
    source = source.replace('void setup(){', 'void cameraBenchSetup(){').replace('void loop(){', 'void cameraBenchLoop(){')
    source = source.replace('c.fb_count=2;', 'c.fb_count=1;')
    source = source.replace('uart-camera-esp32cam-20261108-r6', 'esp32cam-integrada-'+args.revision)
    (stage/'camera_bench.h').write_text(source, encoding='utf-8')
    shutil.copy2(ROOT/'firmware/camera_usb/ov5640_af_firmware.h', stage/'ov5640_af_firmware.h')
    # The source repository contains a placeholder instead of the device's PSK.
    private_password = args.ap_password_file.read_text(encoding='utf-8').strip() if args.ap_password_file.exists() else ''
    if private_password and (not private_password.isascii() or not 8 <= len(private_password) <= 63):
        raise RuntimeError('The local AP password must contain 8-63 ASCII characters')
    for staged_file in stage.iterdir():
        if staged_file.suffix not in ('.ino', '.h'):
            continue
        text = staged_file.read_text(encoding='utf-8-sig')
        if 'CHANGE_ME_ESP_AP' in text:
            if not private_password:
                raise RuntimeError('Create .private/ap-password.txt or pass --ap-password-file before building')
            staged_file.write_text(text.replace('"CHANGE_ME_ESP_AP"', json.dumps(private_password)), encoding='utf-8')
    cmd = [ROOT/'.tools/arduino-cli-stable/arduino-cli.exe', 'compile', '--jobs', '4', '--fqbn', 'esp32:esp32:esp32cam',
           '--build-path', build, '--warnings', 'default']
    if args.profile == 'lyke':
        json_library = Path(os.environ['USERPROFILE'])/'Documents/Arduino/libraries/ArduinoJson'
        if json_library.is_dir():
            cmd += ['--library', json_library]
    if args.optimization == 'lto':
        # Keep the core C panic/backtrace wrapper out of LTO: its Xtensa
        # relocations are incompatible with whole-core LTO in this SDK.
        for key in ('compiler.cpp.extra_flags', 'compiler.c.elf.extra_flags'):
            cmd += ['--build-property', key+'=-flto']
    run(*cmd, stage)
    # Arduino's prebuilt IDF descriptor has a generic project/version. Patch the
    # ELF descriptor and regenerate the entire image/checksum, rather than
    # patching a signed/hash-appended .bin. The OTA header can now reject another
    # ESP32 project's otherwise valid image before writing to the inactive slot.
    elf = build/(NAME+'.ino.elf')
    descriptor = workspace/'appdesc.bin'
    run(gcc/'xtensa-esp32-elf-objcopy.exe', '--dump-section', '.flash.appdesc='+str(descriptor), elf)
    desc = bytearray(descriptor.read_bytes())
    if len(desc) != 256 or int.from_bytes(desc[:4], 'little') != 0xabcd5432:
        raise RuntimeError('Unexpected application descriptor; no binary released')
    desc[16:48] = args.revision.encode().ljust(32, b'\0')
    desc[48:80] = NAME.encode().ljust(32, b'\0')
    descriptor.write_bytes(desc)
    run(gcc/'xtensa-esp32-elf-objcopy.exe', '--update-section', '.flash.appdesc='+str(descriptor), elf)
    binary = out/(NAME+'.ino.bin')
    run(sys.executable, '-m', 'esptool', '--chip', 'esp32', 'elf2image', '--flash-mode', 'dio', '--flash-freq', '80m',
        '--flash-size', '4MB', '--elf-sha256-offset', '0xb0', '-o', binary, elf)
    if binary.stat().st_size > SLOT:
        raise RuntimeError('Application exceeds a 1.5 MiB OTA slot')
    for suffix in ('.ino.bootloader.bin', '.ino.partitions.bin'):
        shutil.copy2(build/(NAME+suffix), out/(NAME+suffix))
    shutil.copy2(sdk/'tools/partitions/boot_app0.bin', out/'boot_app0.bin')
    shutil.copy2(stage/'partitions.csv', out/'partitions.csv')
    shutil.copy2(elf, out/elf.name)
    shutil.copy2(build/(NAME+'.ino.map'), out/(NAME+'.ino.map'))
    manifest = {'target': 'ESP32-D0WD-V3 / AI Thinker', 'chip': 'esp32', 'flash_bytes': 4194304,
                'revision': args.revision, 'optimization': args.optimization, 'slot_bytes': SLOT,
                'core': '3.3.6', 'profile': args.profile, 'files': {}}
    for f in out.glob('*.bin'):
        data = f.read_bytes()
        manifest['files'][f.name] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    (out/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'output': str(out), 'bytes': binary.stat().st_size, 'remaining': SLOT-binary.stat().st_size}))


if __name__ == '__main__':
    main()
