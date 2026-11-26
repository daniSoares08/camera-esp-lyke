"""Run native firmware checks on Windows; never access serial, USB or the API."""
import json
from pathlib import Path
import subprocess
import sys
import argparse

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '.build/lyke_stm32'
RELEASE = OUT / 'lyke-r14'
TESTS = 'firmware/lyke_stm32/tests'
results = []


def run(label, *command):
    result = subprocess.run([str(x) for x in command], cwd=ROOT, capture_output=True, text=True)
    results.append({'check': label, 'command': [str(x) for x in command],
                    'exit_code': result.returncode, 'output': result.stdout + result.stderr})
    print(f"{'PASS' if result.returncode == 0 else 'FAIL'} {label}", flush=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise RuntimeError(label)


def main():
    sys.path.insert(0, str(ROOT / 'firmware/lyke_stm32'))
    from image_codec import pack_image
    import struct

    OUT.mkdir(parents=True, exist_ok=True)
    previews = RELEASE / 'previews'
    previews.mkdir(parents=True, exist_ok=True)
    fixture = bytearray((i * 13 + i // 7) % 256 for i in range(4096))
    struct.pack_into('<II', fixture, 0, 0x200057f0, 0x08002101)
    (RELEASE / 'power-cut-fixture.bin').write_bytes(fixture)
    (RELEASE / 'power-cut-fixture.pkg').write_bytes(pack_image(fixture))

    base = ['-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function']
    run('compile shared decoder', 'gcc', '-O2', '-c', 'firmware/lyke_stm32/vendor/puff.c', '-o', OUT/'puff-host.o')
    for name in ['math_layout', 'interface', 'battery', 'battery_monitor', 'wifi_indicator', 'answer_frame', 'sleep_control', 'power_stop', 'inactivity']:
        run('compile ' + name, 'gcc', '-std=c11', *base, '-Wno-misleading-indentation',
            '-I',previews.parent,f'{TESTS}/test_{name}.c', '-lm', '-o', OUT/f'test_{name}.exe')
        run(name, OUT/f'test_{name}.exe', *([previews] if name != 'battery' else []))
    run('compile answer pager', 'g++', '-std=c++17', *base, f'{TESTS}/test_answer_pager.cpp', '-o', OUT/'test_answer_pager.exe')
    run('answer pager', OUT/'test_answer_pager.exe')
    run('compile loader', 'g++', '-std=c++20', *base, f'{TESTS}/test_boot_store.cpp', OUT/'puff-host.o', '-o', OUT/'test_boot_store.exe')
    run('loader interruption fixture', OUT/'test_boot_store.exe', RELEASE/'power-cut-fixture.pkg', RELEASE/'power-cut-fixture.bin')
    run('loader interruption release', OUT/'test_boot_store.exe', RELEASE/'lyke-stm32-ota.bin', RELEASE/'main.bin')
    run('compile ESP transport', 'g++', '-std=c++17', *base, '-I', 'firmware/lyke_stm32',
        f'{TESTS}/test_stm_transport.cpp', OUT/'puff-host.o', '-o', OUT/'test_stm_transport.exe')
    run('ESP transfer and version guard', OUT/'test_stm_transport.exe', RELEASE/'lyke-stm32-ota.bin')
    run('compile bounded decoder test', 'gcc', '-shared', '-O2', '-Wall', '-Wextra', '-Wno-unused-function',
        f'{TESTS}/image_wrapper.c', 'firmware/lyke_stm32/vendor/puff.c', '-o', OUT/'image_test.dll')
    run('codec bounds/corruption', sys.executable, f'{TESTS}/test_image_codec.py')
    run('USB preservation guards', sys.executable, f'{TESTS}/test_bootstrap_usb.py')
    run('USB prepare only', sys.executable, 'firmware/lyke_stm32/bootstrap_usb.py')
    run('compile rich document', 'g++','-std=c++17',*base,'-Wno-misleading-indentation',
        '-I','C:/Users/campo/Documents/Arduino/libraries/ArduinoJson/src','-I','firmware/calculadora/common',
        f'{TESTS}/test_answer_document.cpp','-o',OUT/'test_answer_document.exe')
    run('rich render and wire',OUT/'test_answer_document.exe',f'{TESTS}/fixtures/answer_graphics.json',previews)


if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--revision',default='lyke-r14');args=parser.parse_args()
    if not args.revision.startswith('lyke-') or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789-.' for c in args.revision):parser.error('Invalid revision')
    RELEASE=OUT/args.revision
    try:
        main()
    finally:
        report = {'hardware_access': False, 'paid_api_calls': 0, 'checks': results,
                  'passed': bool(results) and all(r['exit_code'] == 0 for r in results)}
        (OUT/'native-tests.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
