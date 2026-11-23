"""Draw the proposed wiring from one pin map, as SVG/PNG and a wiring guide."""
from pathlib import Path
import json
import html
import math
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent
BASE = json.loads((OUT / 'PCF8575-no-modulo-4067.json').read_text(encoding='utf-8'))
PORT = {r['board_label']: r['pcf8575_function'] for r in BASE['mapping']}
KEYS = [('L1', 'C0'), ('L2', 'C1'), ('L3', 'C2'), ('L4', 'C3'),
        ('COL1', 'C4'), ('COL2', 'C5'), ('COL3', 'C11'),
        ('COL4', 'C12'), ('COL5', 'C13')]
LCD = [('CS', 'S0'), ('RSE', 'S1'), ('RS', 'S2'),
       ('SCL', 'GPIO15'), ('SI', 'GPIO2'), ('VDD', '3V3'), ('VSS', 'GND')]
RESISTORS = [('4,7 kΩ', 'C8 / GPIO13', '3V3'),
             ('4,7 kΩ', 'C9 / GPIO14', '3V3'),
             ('10 kΩ', 'S0 / CS da tela', '3V3'),
             ('10 kΩ', 'S1 / RSE da tela', '3V3'),
             ('10 kΩ', 'GPIO2 / SI da tela', 'GND')]
used = [p for _, p in KEYS] + [p for _, p in LCD if p in PORT and PORT[p].startswith('P')]
assert len(used) == len(set(used)) == 12
assert all(PORT[p].startswith('P') for p in used)
assert set(p for p, role in PORT.items() if role.startswith('P')) - set(used) == {'S3', 'EN', 'C14', 'C15'}
assert PORT['C8'] == 'SDA' and PORT['C9'] == 'SCL'

plan = dict(board='ESP32-CAM AI Thinker / ESP32 clássico, câmera e PSRAM ativas',
            converted_board='PCF8575 no footprint do CD74HC4067, pino 1 alinhado',
            no_microsd=True, voltage='3.3V', address_7bit='0x20',
            i2c={'GPIO13': 'C8 / SDA', 'GPIO14': 'C9 / SCL'},
            lcd=[{'lcd': name, 'connect_to': pin, 'pcf': PORT.get(pin)} for name, pin in LCD],
            keyboard=[{'keypad': name, 'board_header': pin, 'pcf': PORT[pin]} for name, pin in KEYS],
            ground_headers=['GND', 'C6', 'C7', 'C10'],
            unused_headers=['SIG', 'S3', 'EN', 'C14', 'C15'],
            resistors=[{'value': value, 'from': a, 'to': b} for value, a, b in RESISTORS],
            capacitor='100nF between converted board VCC/GND, close to IC; keep existing if suitable',
            battery={'chemistry': '1S Li-ion/LiPo', 'maximum_voltage': 4.2,
                     'gpio': 33, 'adc': 'ADC1_CH5', 'sense_node': 'VBAT_SENSE',
                     'top_resistor_ohm': 470000, 'bottom_resistor_ohm': 470000,
                     'capacitor_nf': 100, 'maximum_adc_voltage': 2.1,
                     'divider_current_max_ua': 4.2 / 940000 * 1e6,
                     'source_resistance_ohm': 235000, 'settle_ms': 200,
                     'connections': [['BAT+', '470 kΩ / R1', 'VBAT_SENSE'],
                                     ['VBAT_SENSE', '470 kΩ / R2', 'BAT− / GND'],
                                     ['VBAT_SENSE', '100 nF / Cbat', 'BAT− / GND'],
                                     ['VBAT_SENSE', 'fio direto', 'GPIO33']],
                     'access': 'GPIO33 is not on the standard header. Remove/isolate the red status LED branch and identify the GPIO33-side pad by continuity; do not use the 3V3-side pad or GPIO4 white flash LED.',
                     'firmware': 'Input analog, internal pulls disabled; calibrated ADC at 12 dB (legacy 11 dB), settle 200 ms, spaced multisampling; battery voltage = 2 * calibrated ADC voltage. Never drive GPIO33 as LED output.',
                     'power': 'Measure raw battery before the boost/regulator. Keep battery negative and ESP GND common. This divider is not the battery power or charging circuit.'},
            firmware_status='Proposed wiring; integrated keyboard/LCD/battery firmware not yet implemented or bench-tested',
            firmware_constraints=['Use a separate ESP32 I2C controller from camera SCCB; preserve camera bus configuration.',
                                  'Maintain a shared 16-bit PCF output shadow; LCD writes and keypad scanning must preserve each other.',
                                  'PCF quasi-bidirectional column inputs must stay written high.'],
            keypad_contact_order='Logical L1-L4/COL1-COL5; physical ribbon order must be identified')
(OUT / 'ESQUEMA-GERAL-ESP-CAM-PCF8575.json').write_text(json.dumps(plan, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')

W, H = 1800, 2130
im = Image.new('RGB', (W, H), '#f3f6fa')
draw = ImageDraw.Draw(im)
svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">',
       '<rect width="100%" height="100%" fill="#f3f6fa"/>']
INK, GRAY, BLUE, ORANGE, GREEN = '#14253b', '#53647a', '#1766b4', '#a64c00', '#137246'
FONTS = {}

def font(size, bold=False):
    key = (size, bold)
    if key not in FONTS:
        FONTS[key] = ImageFont.truetype('C:/Windows/Fonts/seguisb.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf', size)
    return FONTS[key]

def rect(x, y, w, h, fill='white', stroke='#cad5e2', radius=16):
    draw.rounded_rectangle((x, y, x+w, y+h), radius=radius, fill=fill, outline=stroke, width=2)
    svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{fill}" stroke="{stroke}" stroke-width="2"/>')

def text(x, y, value, size=22, color=INK, bold=False):
    draw.text((x, y), value, font=font(size, bold), fill=color, anchor='lt')
    svg.append(f'<text x="{x}" y="{y}" dominant-baseline="text-before-edge" font-family="Segoe UI,Arial,sans-serif" font-size="{size}" font-weight="{600 if bold else 400}" fill="{color}">{html.escape(value)}</text>')

def arrow(points, color=BLUE, width=4):
    draw.line(points, fill=color, width=width, joint='curve')
    svg.append(f'<polyline points="{" ".join(f"{x},{y}" for x,y in points)}" fill="none" stroke="{color}" stroke-width="{width}"/>')
    a, b = points[-2:]
    theta = math.atan2(b[1]-a[1], b[0]-a[0])
    head = [b, (b[0]-17*math.cos(theta)+8*math.sin(theta), b[1]-17*math.sin(theta)-8*math.cos(theta)),
            (b[0]-17*math.cos(theta)-8*math.sin(theta), b[1]-17*math.sin(theta)+8*math.cos(theta))]
    draw.polygon(head, fill=color)
    svg.append(f'<polygon points="{" ".join(f"{x},{y}" for x,y in head)}" fill="{color}"/>')

def lines(x, y, values, size=22, color=INK, gap=34):
    for i, value in enumerate(values):
        text(x, y+i*gap, value, size, color)

def table(x, y, width, title, headings, rows, widths, accent=BLUE):
    row_h = 39
    height = 109 + len(rows)*row_h
    rect(x, y, width, height)
    text(x+20, y+18, title, 26, accent, True)
    xx = x+20
    for value, col_w in zip(headings, widths):
        text(xx, y+65, value, 20, GRAY, True)
        xx += col_w
    for i, row in enumerate(rows):
        yy = y+102+i*row_h
        if i % 2 == 0:
            rect(x+9, yy-4, width-18, row_h-2, fill='#f3f6fa', stroke='#f3f6fa', radius=4)
        xx = x+20
        for value, col_w in zip(row, widths):
            size = 22
            while draw.textlength(str(value), font=font(size)) > col_w-12 and size > 17:
                size -= 1
            text(xx, yy, str(value), size)
            xx += col_w
    return height

text(50, 29, 'ESP-CAM + teclado 4×5 + LCD GMG12864-06D', 43, INK, True)
text(52, 90, 'Usar os rótulos originais da placa azul, DEPOIS da troca pelo PCF8575 (PF575).', 24, GRAY)

# Overview: arrows represent bundles of wires, not electrical shorted nets.
rect(50, 215, 380, 220, stroke=BLUE)
text(75, 237, 'ESP32-CAM', 32, BLUE, True)
lines(75, 290, ['Câmera: ligação atual', 'UART TX/RX: preservada', '4 GPIOs digitais + GPIO33 ADC'], 22)
rect(700, 215, 420, 220, stroke=BLUE)
text(725, 237, 'Placa convertida', 30, BLUE, True)
lines(725, 288, ['PCF8575 / endereço 0x20', 'C8 = SDA   •   C9 = SCL', 'Teclado + controles da tela'], 23)
rect(1370, 215, 380, 220, stroke=ORANGE)
text(1395, 237, 'LCD 128×64', 30, ORANGE, True)
lines(1395, 288, ['SCL / SI: direto ao ESP', 'CS / RSE / RS: via PCF', 'VDD = 3V3  •  VSS = GND'], 22)
arrow([(430, 335), (700, 335)], BLUE)
text(457, 276, 'GPIO13 → C8', 22, BLUE, True)
text(457, 305, 'GPIO14 → C9', 22, BLUE, True)
arrow([(1120, 335), (1370, 335)], ORANGE)
text(1142, 271, 'S0/S1/S2', 23, ORANGE, True)
text(1142, 303, '→ CS/RSE/RS', 22, ORANGE)
arrow([(235, 215), (235, 160), (1560, 160), (1560, 215)], ORANGE)
rect(530, 137, 780, 44, fill='#f3f6fa', stroke='#f3f6fa', radius=4)
text(550, 143, 'SPI direto: GPIO15 → SCL da tela  |  GPIO2 → SI da tela', 23, ORANGE, True)
rect(700, 505, 420, 115, stroke=GREEN)
text(725, 520, 'Teclado matricial 4×5', 27, GREEN, True)
text(725, 565, '4 linhas + 5 colunas = 9 fios', 23, GREEN)
arrow([(910, 435), (910, 505)], GREEN)
text(945, 456, 'C0–C5 + C11–C13', 22, GREEN)
text(65, 480, 'Sem cartão microSD.', 25, INK, True)
text(65, 522, 'GPIO0 e GPIO16 não entram', 22, GRAY)
text(65, 555, 'neste esquema.', 22, GRAY)

rect(50, 650, 1700, 92, fill='#e7edf5', stroke='#cad5e2')
text(72, 664, '3V3 → VCC da placa convertida + VDD da tela', 24, '#b33333', True)
text(72, 702, 'GND comum → ESP + GND da placa + VSS da tela + C6 + C7 + C10', 24, INK, True)

table(50, 775, 530, 'ESP ↔ placa convertida', ['ESP / destino', 'Rótulo da placa'],
      [('GPIO13', 'C8 (SDA)'), ('GPIO14', 'C9 (SCL)'), ('3V3', 'VCC'),
       ('GND', 'GND'), ('GND', 'C6, C7 e C10'), ('Sem fio', 'SIG / INT')], [220, 270])
table(635, 775, 530, 'Tela → ligar em', ['Pino da tela', 'Destino'],
      [(name, pin + (f' ({PORT[pin]})' if pin in PORT and PORT[pin].startswith('P') else '')) for name, pin in LCD], [180, 310], ORANGE)
table(1220, 775, 530, 'Teclado → placa convertida', ['Teclado', 'Placa / PCF8575'],
      [(name, f'{pin} ({PORT[pin]})') for name, pin in KEYS], [170, 320], GREEN)

table(50, 1260, 1115, 'Resistores e capacitor', ['Valor', 'Uma ponta', 'Outra ponta'],
      RESISTORS + [('100 nF', 'VCC da placa convertida', 'GND, próximo ao PCF')], [170, 475, 420], BLUE)
rect(1220, 1260, 530, 355)
text(1240, 1280, 'Antes de soldar', 27, INK, True)
lines(1240, 1335, ['C0…C15 são nomes na placa azul.',
                   'COL1…COL5 são colunas do teclado.',
                   'Identifique a ordem dos 9 contatos.',
                   'RSE tem pull-up para 3V3, não GND.',
                   'A/K e IC_* da tela ficam sem fio.',
                   'S3, EN, C14 e C15 ficam livres.'], 21, INK, 38)
PURPLE = '#7754a4'
rect(50, 1650, 1700, 345, stroke=PURPLE)
text(75, 1668, 'Bateria 1S • 4,2 V máximo • GPIO33 / ADC1_CH5', 28, PURPLE, True)
text(85, 1760, 'BAT+', 23, '#b33333', True)
arrow([(160, 1776), (230, 1776)], '#b33333', 3)
rect(230, 1763, 110, 26, '#f1eaf7', PURPLE, 2)
text(235, 1724, 'R1 470 kΩ', 22, PURPLE)
arrow([(340, 1776), (480, 1776)], PURPLE, 3)
arrow([(480, 1776), (480, 1720), (745, 1720)], PURPLE, 3)
text(755, 1707, 'GPIO33', 24, PURPLE, True)
text(353, 1800, '2,10 V máx.', 23, PURPLE, True)
arrow([(480, 1776), (600, 1776)], PURPLE, 3)
rect(600, 1763, 110, 26, '#f1eaf7', PURPLE, 2)
text(603, 1724, 'R2 470 kΩ', 22, PURPLE)
arrow([(710, 1776), (865, 1776), (865, 1915), (480, 1915)], GRAY, 3)
arrow([(480, 1776), (480, 1842)], PURPLE, 3)
draw.line([(456, 1842), (504, 1842)], fill=GRAY, width=3)
draw.line([(456, 1855), (504, 1855)], fill=GRAY, width=3)
svg += ['<path d="M456 1842H504M456 1855H504" stroke="'+GRAY+'" stroke-width="3"/>']
arrow([(480, 1855), (480, 1915)], GRAY, 3)
text(525, 1841, 'Cbat 100 nF', 23, GRAY)
text(535, 1927, 'GND comum = BAT− = GND do ESP', 21, GRAY)
draw.ellipse((475, 1771, 485, 1781), fill=PURPLE)
svg.append('<circle cx="480" cy="1776" r="5" fill="'+PURPLE+'"/>')
lines(990, 1724, ['GPIO33: retirar/isolar o LED vermelho.',
                  'Soldar no lado ligado ao GPIO33, não ao 3V3.',
                  'Confirmar por continuidade antes de conectar.',
                  '100 nF perto do GPIO33; divisor: 4,47 µA máx.',
                  'Medir a bateria antes do conversor de tensão.',
                  'Entrada analógica sem pull-up/down; VBAT = 2 × VADC.'], 21, INK, 37)
text(55, 2042, 'Ligação proposta • Alimentação lógica de 3,3 V • Divisor de bateria mede tensão, não alimenta o ESP.', 23, INK, True)
text(55, 2082, 'O firmware atual testa a câmera; teclado, LCD e leitura da bateria ainda precisam de integração. Diagrama fora de escala.', 21, GRAY)
svg.append('</svg>')
stem = OUT / 'ESQUEMA-GERAL-ESP-CAM-PCF8575'
im.save(stem.with_suffix('.png'))
stem.with_suffix('.svg').write_text('\n'.join(svg), encoding='utf-8')

def md_table(headings, rows):
    return '| ' + ' | '.join(headings) + ' |\n|' + '|'.join('---' for _ in headings) + '|\n' + ''.join('| ' + ' | '.join(str(v) for v in row) + ' |\n' for row in rows)

guide = '''# Esquema geral — ESP32-CAM + PCF8575 + teclado 4×5 + GMG12864-06D

Proposta para o ESP32-CAM clássico identificado na COM17, com câmera e PSRAM
ativas, sem cartão microSD. Usar a placa azul apenas depois de substituir
o 4067 pelo PCF8575 no encapsulamento correto e confirmar continuidade.
Todos os nomes C0…C15/S0…S3/EN abaixo são os rótulos antigos dessa placa.

## ESP e alimentação

'''
guide += md_table(['Origem', 'Destino'], [('ESP 3V3', 'VCC da placa + VDD da tela'), ('ESP GND', 'GND da placa + VSS da tela + C6 + C7 + C10'), ('ESP GPIO13', 'C8 / SDA'), ('ESP GPIO14', 'C9 / SCL')])
guide += '\nC6/C7/C10 são A2/A1/A0 do PCF, aterrados para endereço I²C de 7 bits 0x20.\nSIG é INT e não será usado. A alimentação de 3,3 V do ESP depende da fonte\nque já alimenta a placa; este desenho não é um circuito de carga de bateria.\n\n## Tela\n\n'
guide += md_table(['Pino da tela', 'Destino', 'PCF quando aplicável'], [(name, pin, PORT.get(pin, '—')) for name, pin in LCD])
guide += '\nSCL da tela é SPI e vai ao GPIO15; não juntar ao SCL I²C da placa (C9).\nA/K ficam desligados: sem luz de fundo. IC_* da fonte de caracteres não usados.\n\n## Teclado\n\n'
guide += md_table(['Fio lógico do teclado', 'Rótulo da placa', 'PCF'], [(name, pin, PORT[pin]) for name, pin in KEYS])
guide += '\nA ordem física dos nove fios não foi medida. Identificar L1–L4 e COL1–COL5\npor continuidade. Não há VCC/GND dedicado no teclado passivo: seus nove fios\nvão somente às portas listadas. Para ler colunas, o firmware escreve 1 nessas\nportas e usa as fontes fracas internas do PCF8575. Varrer uma linha LOW por vez.\nEste plano considera uma tecla por vez, sem diodos individuais.\n\n## Passivos\n\n'
guide += md_table(['Valor', 'Entre', 'E'], RESISTORS + [('100 nF', 'VCC da placa convertida', 'GND perto do CI')])
guide += '''
Os resistores ligam o sinal ao trilho indicado, não entram em série no fio.
Se já houver capacitor adequado entre VCC/GND na placa, conservá-lo.
Não usar pull-down em RSE com PCF8575. Não confundir o pull-down de GPIO2
com os pull-ups de CS/RSE. S3, EN, C14 e C15 são portas livres, não ligar
EN ao GND. Conferir eventual resistor original de EN caso essa porta venha
a ser utilizada. GPIO12, GPIO4, GPIO0 e GPIO16 não são usados neste esquema;
TX/RX permanecem disponíveis.

## Bateria: divisor 2 × 470 kΩ

Usar GPIO33 / ADC1_CH5, que permite leitura com Wi-Fi ativo. Na ESP32-CAM
AI Thinker, esse GPIO aciona o LED vermelho e não sai no conector de pinos.
Remover/isolar o ramo desse LED e identificar por continuidade o pad que
realmente chega ao GPIO33. Não soldar no lado de 3V3; não usar o LED branco
de flash, que é GPIO4 / ADC2.

```text
BAT+ ── R1 470 kΩ ──┬── GPIO33 (ADC1_CH5)
                    ├── R2 470 kΩ ── BAT− / GND
                    └── Cbat 100 nF ─ BAT− / GND
```

Com bateria em 4,20 V, o ADC recebe 2,10 V; consumo máximo do divisor:
4,47 µA. Preferir resistores de 1%. O capacitor deve ficar perto do ADC:
a resistência equivalente de 235 kΩ torna a fonte de sinal de alta impedância.
Constante RC: 23,5 ms. No firmware, aguardar 200 ms após energizar/acordar,
ler amostras espaçadas e calcular média, usando ADC calibrado com atenuação
de 12 dB (11 dB em APIs antigas). VBAT = 2 × VADC; conferir com multímetro.
Configurar GPIO33 como entrada analógica, sem pulls internos, sem comando de LED.
Percentual por tensão é aproximado e depende da célula e da carga.

Conectar BAT+ à bateria real, antes do boost/regulador; BAT− e GND do ESP
devem estar unidos. O divisor não é circuito de alimentação nem carregador.
Não ligar os 4,2 V diretamente ao pino 3V3. Evitar manter BAT+ no divisor
com o ESP totalmente sem alimentação para não injetar corrente pela entrada;
se houver chave geral, o topo do divisor pode ficar após essa chave, ainda
antes do conversor de tensão, para desligar junto com o aparelho.

## Estado e fontes

Ligação planejada, sem ensaio de bancada de teclado/LCD. Em 09/11 foi instalada
a base `cam-ota-r2-wifi`, com fila de fotos, OTA e drivers preliminares PCF/LCD.
O teste físico foi sem periféricos; menu e mapa físico do teclado seguem pendentes.
Os três controles do LCD passam pelo PCF; clock e dados são gerados no ESP.
No core 3.3.6 desta bancada, o SCCB da câmera usa I2C1; o PCF usa TwoWire(0),
mantendo SDA=13/SCL=14. Essa separação foi conferida no SDK e no diagnóstico.
Manter uma imagem de
16 bits dos estados do PCF e serializar varredura/controle do LCD: atualizar a
tela não pode sobrescrever os bits de linhas/colunas, nem o contrário.

- PCF8575: https://www.ti.com/lit/ds/symlink/pcf8575.pdf
- CD74HC4067: https://www.ti.com/lit/ds/symlink/cd74hc4067.pdf
- Tela: https://ae-pic-a1.aliexpress-media.com/kf/S3dd8f051c7fc44a2873b1aee32568c34c.pdf
- ESP32 (GPIO2 / boot): https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
- ESP32-CAM (LED vermelho / GPIO33): https://raw.githubusercontent.com/SeeedDocument/forum_doc/master/reg/ESP32_CAM_V1.6.pdf
- ADC e Wi-Fi: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/peripherals/adc_oneshot.html
- ADC, calibração e capacitor: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/peripherals/adc_calibration.html
'''
stem.with_suffix('.md').write_text(guide, encoding='utf-8')
print('Generated SVG, PNG, JSON and wiring guide; 12 unique PCF ports allocated, 4 free.')
