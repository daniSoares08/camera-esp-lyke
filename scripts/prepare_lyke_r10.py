"""Assemble the local/private Lyke R10 test kit. No hardware/network access."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'firmware/lyke_stm32'))
from bootstrap_usb import verify_release
from image_codec import unpack_image


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    stm = ROOT / '.build/lyke_stm32/lyke-r10a'
    esp = ROOT / 'firmware/esp32cam_integrada/bin/lyke-r8'
    out = ROOT / 'output/firmware/lyke-r10-pronto'
    manifest, _ = verify_release(stm)
    package = (stm/'lyke-stm32-ota.bin').read_bytes()
    if digest(package) != manifest['artifacts']['lyke-stm32-ota.bin']['sha256']:
        raise ValueError('Pacote STM diferente do manifesto')
    if digest(unpack_image(package)) != manifest['app_sha256']:
        raise ValueError('Aplicacao compactada diferente do manifesto')
    esp_manifest = json.loads((esp/'manifest.json').read_text())
    app_name = 'esp32cam_integrada.ino.bin'
    app = (esp/app_name).read_bytes()
    if (esp_manifest['chip'] != 'esp32' or esp_manifest['profile'] != 'lyke'
            or esp_manifest['revision'] != 'lyke-r8' or len(app) > esp_manifest['slot_bytes']
            or digest(app) != esp_manifest['files'][app_name]['sha256']):
        raise ValueError('Aplicacao ESP incompativel ou diferente do manifesto')
    report = json.loads((ROOT/'.build/lyke_stm32/native-tests.json').read_text())
    if not report['passed'] or report['hardware_access'] or report['paid_api_calls']:
        raise ValueError('Verificar os testes nativos antes de montar o pacote')
    for name in ['STM32','ESP32']:
        (out/name).mkdir(parents=True,exist_ok=True)
    for name in ['lyke-stm32-dfu.bin','lyke-stm32-ota.bin','manifest.json']:
        shutil.copyfile(stm/name,out/'STM32'/name)
    for name in [app_name,'manifest.json']:
        shutil.copyfile(esp/name,out/'ESP32'/name)
    shutil.copyfile(ROOT/'output/previews/lyke-r10-telas.png',out/'telas.png')
    shutil.copyfile(ROOT/'.build/lyke_stm32/native-tests.json',out/'testes-nativos.json')
    instructions = (ROOT/'firmware/lyke_stm32/PRONTO-PARA-TESTAR-R10.md').read_text(encoding='utf-8')
    instructions = instructions.replace('../../output/previews/lyke-r10-telas.png','telas.png')
    instructions = instructions.replace('docs/VALIDACAO-R10-2026-11-23.md','VALIDACAO.md')
    shutil.copyfile(ROOT/'docs/VALIDACAO-R10-2026-11-23.md',out/'VALIDACAO.md')
    (out/'INSTRUCOES.md').write_text(instructions,encoding='utf-8')
    (out/'LEIA-ME.md').write_text('''# Lyke — pacote local para teste

Instalados e conferidos em 23/11/2026: STM32 R10a + ESP Lyke R8.
Base STM R10 instalada por USB; R10a instalada depois pelo Wi-Fi/UART existente.
R10a tem as mesmas funções da R10, com identificador alterado para comprovar a OTA.

Leia [as instruções](INSTRUCOES.md) antes de usar os arquivos.
Os comandos das instruções partem da pasta C:\\Projects\\CAMERA_ESP.

| Arquivo | Uso |
|---|---|
| STM32/lyke-stm32-dfu.bin | Primeira instalação USB: carregador V2 e aplicação juntos |
| STM32/lyke-stm32-ota.bin | Atualizações seguintes pelo portal do ESP Lyke R8 |
| ESP32/esp32cam_integrada.ino.bin | Atualização da aplicação do ESP pelo portal atual |
| telas.png | Simulações com o código real de desenho do LCD |
| testes-nativos.json | Testes no computador, sem USB ou API paga |

Não misture os três binários. Os manifestos registram os hashes; o manifesto
do ESP também lista arquivos auxiliares da compilação que não entram neste
pacote de atualização de aplicação. Não se gravam tabela, NVS ou bootloader do ESP.

Os binários são para uso local privado. Não publicar este pacote.
''',encoding='utf-8')
    files = {str(p.relative_to(out)).replace('\\','/'):{'bytes':p.stat().st_size,'sha256':digest(p.read_bytes())}
             for p in sorted(out.rglob('*')) if p.is_file() and p.name != 'release.json'}
    physical = json.loads((ROOT/'.private/lyke-cloud/device-stm-r10a-wifi.json').read_text())
    confirmation = json.loads((ROOT/'.private/lyke-cloud/r10-physical-confirmation.json').read_text())
    if (not physical['success'] or physical['after']['stm_revision'] != manifest['revision']
            or physical['binary_sha256'] != digest(package)
            or physical['after']['image_sha256'] != app[-32:].hex()):
        raise ValueError('Confirmar o teste fisico da revisao atual')
    release = {'date':'2026-11-23','timezone':'America/Sao_Paulo','hardware_access_by_packager':False,
               'stm_revision':'lyke-r10a','esp_revision':'lyke-r8','native_checks_passed':True,
               'stm_wifi_update_physically_verified':True,'update_seconds':physical['seconds'],
               'physical_lcd_notation_confirmed_by_user':confirmation['root_and_fraction_display_fit_original_window'],
               'files':files}
    (out/'release.json').write_text(json.dumps(release,indent=2)+'\n',encoding='utf-8')
    archive = out.parent/'Lyke-R10-pronto.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(out.rglob('*')):
            if p.is_file():z.write(p,str(Path(out.name)/p.relative_to(out)))
    print(json.dumps({'ready':True,'directory':str(out),'zip':str(archive),
                      'bytes':archive.stat().st_size,'sha256':digest(archive.read_bytes())}))


if __name__ == '__main__':
    main()
