"""Confere o firmware gravado por USB e salva buffers realmente enviados ao LCD."""
import argparse
import json
from pathlib import Path
import re
import time

import serial
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'outputs' / 'teste_lcm12864_v4'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True)
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    log = []
    with serial.Serial(args.port, 115200, timeout=.15) as port:
        def exchange(command, seconds=.25):
            port.write((command + '\n').encode())
            deadline = time.monotonic() + seconds
            lines = []
            while time.monotonic() < deadline:
                line = port.readline().decode(errors='replace').strip()
                if line:
                    lines.append(line)
                    log.append(line)
            return lines

        def status():
            lines = exchange('s')
            matches = [x for x in lines if x.startswith('STATUS ')]
            assert matches, lines
            return dict(re.findall(r'(\w+)=(\S+)', matches[-1]))

        try:
            time.sleep(.4)
            initial = status()
            assert initial['build'] == 'lcm12864-demo-v4', initial
            assert initial['csLow'] == '1' and initial['swap'] == '0', initial
            if initial['paused'] == '1':
                exchange('p')
            if initial['invert'] == '1':
                exchange('i')
            if initial['rotate'] == '1':
                exchange('r')
            states, images = [], []
            for key in '1234567890':
                exchange(key, .65)
                exchange('p')
                first = status()
                time.sleep(.15)
                second = status()
                assert first['hash'] == second['hash'], (first, second)
                assert int(second['frames']) > int(first['frames'])
                assert second['auto'] == '0' and second['paused'] == '1'
                lines = exchange('b', .4)
                raw = bytes.fromhex(next(x[7:] for x in lines if x.startswith('BUFFER ')))
                assert len(raw) == 1024
                image = Image.new('RGB', (128, 64), '#101c15')
                for y in range(64):
                    for x in range(128):
                        if raw[(y // 8) * 128 + x] & (1 << (y % 8)):
                            image.putpixel((x, y), (190, 239, 161))
                number = int(second['scene'])
                image.resize((512, 256), Image.Resampling.NEAREST).save(OUT / f'cena_{number:02}.png')
                images.append(image)
                states.append(second)
                exchange('p')
            # Rotacao, inversao e configuracao de CS devem ser reversiveis.
            exchange('1'); exchange('p')
            original = status()
            exchange('i'); inverted = status()
            assert original['hash'] != inverted['hash']
            exchange('i'); assert status()['hash'] == original['hash']
            for key, field in [('r', 'rotate'), ('c', 'csLow'), ('w', 'swap')]:
                baseline = status()[field]
                exchange(key); assert status()[field] == str(1 - int(baseline))
                exchange(key); assert status()[field] == baseline
            exchange('d')
            reset_display = status()
            assert reset_display['hash'] == original['hash']
            exchange('p'); exchange('a')
            before = status()
            exchange('s', 13)
            after = status()
            assert after['scene'] == '2' and after['auto'] == '1' and after['paused'] == '0', after
            assert int(after['uptime_ms']) > int(before['uptime_ms'])
            assert int(after['frames']) > int(before['frames']) + 100
            # Deixa o robo como primeira cena, em ciclo automatico.
            exchange('1'); exchange('a')
            final = status()
            sheet = Image.new('RGB', (800, 5 * 225), '#f2f4ef')
            draw = ImageDraw.Draw(sheet)
            labels = ['Robo', 'Radar', 'Cubo 3D', 'Estrelas', 'Pong', 'Ondas',
                      'Equalizador', 'Relogio', 'Pixels (fase apagada)', 'CS1 / CS2']
            for i, image in enumerate(images):
                x, y = (i % 2) * 400 + 8, (i // 2) * 225 + 6
                draw.text((x, y), f'{i + 1:02} - {labels[i]}', fill='#18231c')
                sheet.paste(image.resize((384, 192), Image.Resampling.NEAREST), (x, y + 18))
            sheet.save(OUT / 'previa_buffers.png')
            report = {'initial': initial, 'scenes': states, 'inverted': inverted,
                      'automatic_before': before, 'automatic_after': after, 'final': final,
                      'checks': '10 cenas, pausa, inversao, rotacao, polaridade, metades, reinicializacao LCD, troca automatica',
                      'limit': 'Buffers e execucao verificados; imagem fisica no LCD depende de inspecao visual.'}
            (OUT / 'verificacao.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
            print(json.dumps(report, indent=2))
        finally:
            (OUT / 'serial.log').write_text('\n'.join(log), encoding='utf-8')


if __name__ == '__main__':
    main()
