"""One initial USB install of the resident loader + app, preserving configuration.

Prepare/validate only: python bootstrap_usb.py --release <directory>
Install later: add --install --serial <observed DFU serial> --tool <dfu-util.exe>
No mass erase, option-byte write, or connection to the ESP serial adapter.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
KEEP = 0x1E800
FLASH = 128 * 1024


def internal_flash_matches(line):
    """dfu-util may render 64*02Kg or zero-padded 064*002Kg."""
    name = re.search(r'name="([^"]+)"', line)
    if not name:
        return False
    descriptor = re.sub(r'\s', '', name[1])
    region = re.fullmatch(r'@InternalFlash/0x08000000/(\d+)\*(\d+)K[gG]', descriptor)
    return bool(region and int(region[1]) == 64 and int(region[2]) == 2)


def verify_release(release):
    manifest = json.loads((release/'manifest.json').read_text())
    if (manifest['target'] != 'STM32G431CBU6' or manifest['app_origin'] != '0x08002000'
            or manifest['stage_origin'] != '0x08012000' or manifest.get('preserve_from') != '0x0801e800'
            or manifest.get('loader_protocol') != 'LYKE BOOT V2'):
        raise ValueError('Manifesto de outro alvo ou mapa de memoria')
    data = (release/'lyke-stm32-dfu.bin').read_bytes()
    record = manifest['artifacts']['lyke-stm32-dfu.bin']
    if len(data) != KEEP or len(data) != record['bytes'] or hashlib.sha256(data).hexdigest() != record['sha256']:
        raise ValueError('Imagem USB nao coincide com o manifesto')
    for at, limit in [(0, 0x2000), (0x2000, 0x2000+manifest['app_bytes'])]:
        stack, pc = struct.unpack_from('<II', data, at)
        if stack != 0x200057F0 or not pc & 1 or not 0x08000000+at+8 <= pc < 0x08000000+limit:
            raise ValueError('Vetores invalidos')
    if any(x != 255 for x in data[0x12000:]):
        raise ValueError('A instalacao USB precisa limpar staging/metadados antigos')
    return manifest, data


def verify_readback(before, after, expected):
    if len(before) != FLASH or len(after) != FLASH:
        raise ValueError('Leitura incompleta da flash de 128 KiB')
    if after[:KEEP] != expected:
        raise ValueError('Programa/carregador nao conferem na releitura')
    if before[KEEP:] != after[KEEP:]:
        raise ValueError('Mapa/preferencias diferentes: preservar backup e interromper')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--release', type=Path, default=ROOT/'.build/lyke_stm32/lyke-r12')
    parser.add_argument('--install', action='store_true')
    parser.add_argument('--serial', help='Serial observado agora em dfu-util -l')
    parser.add_argument('--tool', default=shutil.which('dfu-util'))
    args = parser.parse_args()
    release=args.release.resolve();manifest,expected=verify_release(release)
    if not args.install:
        print(json.dumps({'ready':True,'revision':manifest['revision'],'bytes':len(expected),
                          'preserves':'0x0801e800..0x0801ffff','hardware_access':False}))
        return
    if not args.tool or not args.serial or not re.fullmatch('[A-Fa-f0-9]{8,32}',args.serial):
        parser.error('--install exige --tool e --serial reidentificado')
    listing=subprocess.run([args.tool,'-l'],capture_output=True,text=True,check=True)
    matches=[line for line in listing.stdout.splitlines() if '0483:df11' in line.lower()
             and f'serial="{args.serial}"' in line and 'alt=0,' in line]
    if len(matches)!=1 or not internal_flash_matches(matches[0]):
        raise ValueError('STM32/serial/flash de 128 KiB nao identificados unicamente')
    backup=ROOT/'.private'/('stm32_lyke_r10_'+time.strftime('%Y%m%d_%H%M%S'))
    backup.mkdir(parents=True,exist_ok=False)
    base=[args.tool,'-d','0483:df11','-S',args.serial,'-a','0']
    def run(label,*extra):
        result=subprocess.run(base+list(extra),capture_output=True)
        (backup/(label+'.log')).write_bytes(result.stdout+b'\n'+result.stderr)
        result.check_returncode()
    run('before','-s','0x08000000:131072','-U',str(backup/'before.bin'))
    before=(backup/'before.bin').read_bytes()
    if len(before)!=FLASH: raise ValueError('Backup incompleto; nao gravar')
    run('download','-s','0x08000000','-D',str(release/'lyke-stm32-dfu.bin'))
    run('after','-s','0x08000000:131072','-U',str(backup/'after.bin'))
    after=(backup/'after.bin').read_bytes();verify_readback(before,after,expected)
    report={'success':True,'serial':args.serial,'revision':manifest['revision'],
            'sha256':hashlib.sha256(expected).hexdigest(),'configuration_preserved':True}
    (backup/'result.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report))
    print('Solte BOOT0 e pressione RST para iniciar. Depois validar a OTA pelo ESP.')


if __name__=='__main__':main()
