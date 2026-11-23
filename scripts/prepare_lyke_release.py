"""Local/private release package; no device, credentials or network operations."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'firmware/lyke_stm32'))
from bootstrap_usb import verify_release
from image_codec import unpack_image

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--revision',default='lyke-r11')
    args=parser.parse_args()
    if not args.revision.startswith('lyke-') or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789-.' for c in args.revision):
        parser.error('Invalid revision')
    source=ROOT/'.build/lyke_stm32'/args.revision
    manifest,_=verify_release(source)
    package=(source/'lyke-stm32-ota.bin').read_bytes()
    sha=lambda data:hashlib.sha256(data).hexdigest()
    assert sha(package)==manifest['artifacts']['lyke-stm32-ota.bin']['sha256']
    assert sha(unpack_image(package))==manifest['app_sha256']
    out=ROOT/'output/firmware'/(args.revision+'-final')
    (out/'STM32').mkdir(parents=True,exist_ok=True)
    for name in ['manifest.json','lyke-stm32-dfu.bin','lyke-stm32-ota.bin']:
        shutil.copyfile(source/name,out/'STM32'/name)
    (out/'ESP32').mkdir(exist_ok=True)
    esp=ROOT/'firmware/esp32cam_integrada/bin/lyke-r8'
    for name in ['manifest.json','esp32cam_integrada.ino.bin']:
        shutil.copyfile(esp/name,out/'ESP32'/name)
    shutil.copyfile(ROOT/'.build/lyke_stm32/native-tests.json',out/'testes-nativos.json')
    (out/'LEIA-ME.md').write_text('''# Lyke - pacote local privado

STM32 R11 + ESP Lyke R8, instalados no aparelho do projeto.
Use STM32/lyke-stm32-ota.bin na linha STM32 do portal Calc-Lyke.
O DFU é só para instalação inicial/recuperação do STM pelo USB, com backup.
ESP32/esp32cam_integrada.ino.bin é apenas para a linha ESP32-CAM.
Não misturar arquivos nem publicar estes binários. Os manifestos contêm hashes.
O manifesto ESP também lista auxiliares de compilação que não estão neste pacote.
Manual final em output/pdf/Manual-Lyke.pdf e Manual-Lyke.md no workspace.
''',encoding='utf-8')
    files={str(p.relative_to(out)).replace('\\','/'):{'bytes':p.stat().st_size,'sha256':sha(p.read_bytes())}
           for p in out.rglob('*') if p.is_file() and p.name!='release.json'}
    (out/'release.json').write_text(json.dumps({'date':'2026-11-23','stm':manifest['revision'],'esp':'lyke-r8','private_firmware_binaries':True,'files':files},indent=2)+'\n')
    archive=out.parent/(out.name+'.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(out.rglob('*')):
            if p.is_file():z.write(p,str(Path(out.name)/p.relative_to(out)))
    print(json.dumps({'folder':str(out),'zip':str(archive),'sha256':sha(archive.read_bytes())}))

if __name__=='__main__':main()
