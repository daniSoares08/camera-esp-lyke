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
    parser.add_argument('--revision',default='lyke-r13')
    parser.add_argument('--esp-revision',default='lyke-r11')
    args=parser.parse_args()
    for value in [args.revision,args.esp_revision]:
        if not value.startswith('lyke-') or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789-.' for c in value):
            parser.error('Invalid revision')
    source=ROOT/'.build/lyke_stm32'/args.revision
    manifest,_=verify_release(source)
    package=(source/'lyke-stm32-ota.bin').read_bytes()
    sha=lambda data:hashlib.sha256(data).hexdigest()
    assert sha(package)==manifest['artifacts']['lyke-stm32-ota.bin']['sha256']
    assert sha(unpack_image(package))==manifest['app_sha256']
    installed=False
    proof=ROOT/'.private/lyke-cloud'/('device-stm-'+args.revision.replace('lyke-','')+'-wifi.json')
    if proof.exists():
        p=json.loads(proof.read_text());installed=p.get('success') and p.get('binary_sha256')==sha(package)
    esp_proof=ROOT/'.private/lyke-cloud'/('device-'+args.esp_revision.replace('lyke-','')+'-ota.json')
    esp_installed=False
    if esp_proof.exists():
        ep=json.loads(esp_proof.read_text())
        esp_installed=bool(ep.get('success') and ep.get('after',{}).get('revision')==args.esp_revision)
    out=ROOT/'output/firmware'/(args.revision+'-esp-'+args.esp_revision.replace('lyke-','')+('-final' if installed and esp_installed else '-preparado'))
    (out/'STM32').mkdir(parents=True,exist_ok=True)
    for name in ['manifest.json','lyke-stm32-dfu.bin','lyke-stm32-ota.bin']:
        shutil.copyfile(source/name,out/'STM32'/name)
    (out/'ESP32').mkdir(exist_ok=True)
    esp=ROOT/'firmware/esp32cam_integrada/bin'/args.esp_revision
    esp_manifest=json.loads((esp/'manifest.json').read_text())
    assert esp_manifest['chip']=='esp32' and esp_manifest['profile']=='lyke'
    esp_data=(esp/'esp32cam_integrada.ino.bin').read_bytes()
    assert len(esp_data)<=esp_manifest['slot_bytes'] and sha(esp_data)==esp_manifest['files']['esp32cam_integrada.ino.bin']['sha256']
    if esp_installed:
        assert ep.get('binary_sha256')==sha(esp_data),'Installed ESP proof refers to a different binary'
    for name in ['manifest.json','esp32cam_integrada.ino.bin']:
        shutil.copyfile(esp/name,out/'ESP32'/name)
    shutil.copyfile(ROOT/'.build/lyke_stm32/native-tests.json',out/'testes-nativos.json')
    focused=ROOT/'.build/lyke_esp32'/(args.esp_revision+'-validation.json')
    if focused.exists():shutil.copyfile(focused,out/'testes-esp.json')
    (out/'LEIA-ME.md').write_text(f'''# Lyke - pacote local privado

STM32 {args.revision} + ESP {args.esp_revision}.
Estado STM: {'VERIFICADO NO APARELHO' if installed else 'PREPARADO, AINDA NAO INSTALADO'}.
Estado ESP: {'VERIFICADO NO APARELHO' if esp_installed else 'PREPARADO, AINDA NAO INSTALADO'}.
Use STM32/lyke-stm32-ota.bin na linha STM32 do portal Calc-Lyke.
O DFU é só para instalação inicial/recuperação do STM pelo USB, com backup.
ESP32/esp32cam_integrada.ino.bin é apenas para a linha ESP32-CAM.
Não misturar arquivos nem publicar estes binários. Os manifestos contêm hashes.
O manifesto ESP também lista auxiliares de compilação que não estão neste pacote.
Manual final em output/pdf/Manual-Lyke.pdf e Manual-Lyke.md no workspace.
Para sono profundo, atualizar os DOIS: ESP R11 primeiro; depois STM R13 pelo portal.
O carregador V2 instalado e o mapa de teclas permanecem; nao usar DFU sem necessidade.
As revisoes ESP/STM sao independentes, nao arquivos intercambiaveis.
Guia: docs/REVISAO-R13-SONO-PROFUNDO.md no workspace.
Para o novo perfil de energia, Calc-Lyke abre no boot/menu e encerra apos
dois minutos sem atividade no portal; roteador continua em modo economico.
Fotos pausam o radio e restauram o estado anterior automaticamente.
SHIFT+AC apaga LCD e coordena ESP deep-sleep + STM STOP1; somente MODE desperta.
Fotos temporarias ainda nao enviadas se perdem; resposta salva/configuracoes ficam.
Nao ha Wi-Fi, captura, atualizacao nem medicao periodica durante o sono.
''',encoding='utf-8')
    files={str(p.relative_to(out)).replace('\\','/'):{'bytes':p.stat().st_size,'sha256':sha(p.read_bytes())}
           for p in out.rglob('*') if p.is_file() and p.name!='release.json'}
    (out/'release.json').write_text(json.dumps({'date':'2026-11-25','stm':manifest['revision'],'esp':args.esp_revision,'stm_installed_proof':bool(installed),'esp_installed_proof':esp_installed,'private_firmware_binaries':True,'files':files},indent=2)+'\n')
    archive=out.parent/(out.name+'.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(out.rglob('*')):
            if p.is_file():z.write(p,str(Path(out.name)/p.relative_to(out)))
    print(json.dumps({'folder':str(out),'zip':str(archive),'sha256':sha(archive.read_bytes())}))

if __name__=='__main__':main()
