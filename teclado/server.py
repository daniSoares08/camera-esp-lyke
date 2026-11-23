"""HP48 keyboard mapping UI, local-only serial bridge. No third-party web deps."""
import argparse
import copy
import csv
import io
import json
from pathlib import Path
import secrets
import threading
import time
import urllib.request
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

import serial
from serial.tools import list_ports

ROOT = Path(__file__).resolve().parent
DATA = ROOT.parent / 'outputs' / 'mapa_teclado'
FIRMWARE = 'hp48-mapper-uart-v3'
PINS = [0, 1, 2, 3, 14, 19, 20, 21, 38, 39, 40, 41, 42, 45, 46, 47, 48]
ROWS = [
    [('A','A'),('B','B'),('C','C'),('D','D'),('E','E'),('F','F')],
    [('G','MTH'),('H','PRG'),('I','CST'),('J','VAR'),('K','▲'),('L','NXT')],
    [('M',"'"),('N','STO'),('O','EVAL'),('P','◀'),('Q','▼'),('R','▶')],
    [('S','SIN'),('T','COS'),('U','TAN'),('V','√x'),('W','yˣ'),('X','1/x')],
    [('Y','ENTER'),('Z','+/−'),('EEX','EEX'),('DEL','DEL'),('BACKSPACE','⌫')],
    [('ALPHA','α'),('7','7'),('8','8'),('9','9'),('DIV','÷')],
    [('LSHIFT','⇦'),('4','4'),('5','5'),('6','6'),('MUL','×')],
    [('RSHIFT','⇨'),('1','1'),('2','2'),('3','3'),('SUB','−')],
    [('ON','ON'),('0','0'),('DOT','.'),('SPC','SPC'),('ADD','+')],
]
KEYS = [dict(id=k, label=label, row=r, col=c, alpha=k if len(k)==1 and k.isalpha() else '')
        for r, row in enumerate(ROWS) for c, (k, label) in enumerate(row)]


class Mapper:
    def __init__(self, port=None, data_dir=DATA):
        self.lock = threading.RLock()
        self.data_dir = Path(data_dir)
        self.path = self.data_dir / 'mapa.json'
        self.mapping = {}
        self.target = 0
        self.armed = False
        self.connected = False
        self.port = port or ''
        self.preferred = port
        self.message = 'Aguardando firmware pela USB/UART…'
        self.live = None
        self.events = []
        self.info = {}
        self.last_hello = 0
        self.stop = threading.Event()
        self.link = None
        self.log_id = 0
        self.load()

    def load(self):
        if not self.path.exists():
            return
        saved = json.loads(self.path.read_text(encoding='utf-8'))
        for key, value in saved.get('mapping', {}).items():
            if key not in {k['id'] for k in KEYS}:
                raise ValueError('Tecla desconhecida no mapa salvo')
            if value is not None:
                a, b = value['a'], value['b']
                if a not in PINS or b not in PINS + [-1] or a == b:
                    raise ValueError('Pinos inválidos no mapa salvo')
        self.mapping = saved.get('mapping', {})
        self.target = self.next_missing()

    def next_missing(self):
        return next((i for i, k in enumerate(KEYS) if k['id'] not in self.mapping), len(KEYS))

    def document(self):
        return dict(schema=1, firmware=FIRMWARE, transport='UART', pins=PINS,
                    idle_low_pins=self.info.get('idle_low_pins', []),
                    updated_at=time.strftime('%Y-%m-%dT%H:%M:%S%z'),
                    complete=all(k['id'] in self.mapping and self.mapping[k['id']] is not None for k in KEYS),
                    keys=KEYS, mapping=self.mapping)

    def save(self):
        self.data_dir.mkdir(parents=True, exist_ok=True)
        temp = self.path.with_suffix('.tmp')
        temp.write_text(json.dumps(self.document(), ensure_ascii=False, indent=2), encoding='utf-8')
        temp.replace(self.path)

    def log(self, text, kind='info'):
        self.log_id += 1
        self.events.insert(0, dict(id=self.log_id, time=time.strftime('%H:%M:%S'), text=text, kind=kind))
        del self.events[80:]

    def state(self):
        with self.lock:
            return copy.deepcopy(dict(connected=self.connected, port=self.port, armed=self.armed,
                target=self.target, keys=KEYS, mapping=self.mapping, message=self.message,
                live=self.live, events=self.events[:20], info=self.info, pins=PINS,
                save_path=str(self.path), mapped=sum(v is not None for v in self.mapping.values()),
                skipped=sum(v is None for v in self.mapping.values())))

    def ingest(self, event):
        with self.lock:
            if not isinstance(event, dict):
                return
            kind = event.get('type')
            if kind == 'hello':
                if event.get('firmware') != FIRMWARE or event.get('pins') != PINS:
                    self.connected = False
                    self.message = 'Firmware diferente. Grave mapear_teclado antes de continuar.'
                    return
                if not self.connected:
                    self.log('ESP conectado em ' + self.port)
                    self.message = 'Pronto. Inicie e aperte a tecla destacada.'
                previous = self.info
                self.connected = True
                self.last_hello = time.monotonic()
                self.info = event
                self.data_dir.mkdir(parents=True, exist_ok=True)
                with (self.data_dir / 'diagnostico.jsonl').open('a', encoding='utf-8') as capture:
                    capture.write(json.dumps(event) + '\n')
                rebooted = event.get('uptime_ms', 0) < previous.get('uptime_ms', 0)
                if not event.get('enabled') and self.armed and (previous.get('enabled') or rebooted):
                    self.armed = False
                    self.message = 'ESP reiniciado. Clique em Iniciar para continuar.'
                return
            if not self.connected:
                return
            if kind == 'up':
                self.live = None
                return
            if kind == 'ambiguous':
                self.live = None
                self.message = 'Mais de um contato: solte TODAS as teclas e tente novamente.'
                self.log(self.message, 'warn')
                return
            if kind != 'down':
                return
            a, b = event.get('a'), event.get('b')
            if a not in PINS or b not in PINS + [-1] or a == b:
                return
            if b >= 0 and a > b:
                a, b = b, a
            pair = dict(a=a, b=b)
            self.live = pair
            name = f'GPIO{a} ↔ ' + ('GND' if b == -1 else f'GPIO{b}')
            matches = [key for key, val in self.mapping.items() if val == pair]
            if not self.armed:
                self.message = ('Teste: ' + ', '.join(matches) + ' · ' + name) if matches else 'Contato detectado: ' + name
                self.log(self.message)
                return
            if self.target >= len(KEYS):
                self.armed = False
                return
            key = KEYS[self.target]['id']
            if matches and key not in matches:
                self.message = 'Contato já pertence a ' + ', '.join(matches) + '. A posição não avançou.'
                self.log(self.message, 'warn')
                return
            previous = dict(self.mapping)
            self.mapping[key] = pair
            try:
                self.save()
            except OSError:
                self.mapping = previous
                self.armed = False
                self.message = 'Falha ao salvar. Mapeamento pausado; confira a pasta de saída.'
                self.log(self.message, 'warn')
                return
            self.log(key + ' · ' + name, 'ok')
            self.target = self.next_missing()
            if self.target == len(KEYS):
                self.armed = False
                self.message = 'Sequência encerrada. Confira as teclas e exporte o mapa.'
            else:
                self.message = 'Registrado ' + key + '. Solte e aperte a próxima tecla.'

    def send(self, command):
        if self.link and self.connected:
            self.link.write((command + '\n').encode())

    def action(self, action, index=None):
        with self.lock:
            if action == 'start':
                if not self.connected:
                    raise ValueError('ESP desconectado')
                if self.target >= len(KEYS):
                    raise ValueError('Selecione uma tecla para remapear')
                self.send('START')
                self.armed = True
                self.message = 'Solte todas as teclas e aperte ' + KEYS[self.target]['id'] + '.'
            elif action == 'pause':
                self.armed = False
                self.message = 'Pausado. Os contatos continuam visíveis para teste.'
            elif action == 'select':
                if type(index) is not int or not 0 <= index < len(KEYS):
                    raise ValueError('Posição inválida')
                self.target = index
                self.armed = False
                self.message = 'Selecionado ' + KEYS[index]['id'] + '. Clique em Iniciar para remapear.'
            elif action == 'undo':
                self.armed = False
                previous = [i for i, k in enumerate(KEYS) if i < self.target and k['id'] in self.mapping]
                if previous:
                    self.target = previous[-1]
                    self.mapping.pop(KEYS[self.target]['id'], None)
                    self.save()
                self.message = 'Correção pronta. Clique em Iniciar para registrar a posição destacada.'
            elif action == 'skip':
                if self.target < len(KEYS):
                    self.mapping[KEYS[self.target]['id']] = None
                    self.save()
                    self.target = self.next_missing()
                    if self.target == len(KEYS):
                        self.armed = False
                    self.message = 'Posição marcada como pendente; poderá ser remapeada depois.'
            elif action == 'test':
                if not self.connected:
                    raise ValueError('ESP desconectado')
                self.armed = False
                self.send('START')
                self.message = 'Modo teste. Aperte qualquer tecla para identificar seu contato.'
            else:
                raise ValueError('Ação desconhecida')

    def serial_loop(self):
        while not self.stop.is_set():
            ports = list(list_ports.comports())
            candidates = [p for p in ports if p.device == self.preferred] if self.preferred else [p for p in ports if p.vid in (0x1A86, 0x10C4, 0x0403)]
            if not candidates:
                time.sleep(1)
                continue
            try:
                with serial.Serial(candidates[0].device, 115200, timeout=.15, write_timeout=1, dsrdtr=False) as link:
                    link.dtr = False
                    link.rts = False
                    with self.lock:
                        self.link = link
                        self.port = link.port
                    link.write(b'HELLO\n')
                    opened = time.monotonic()
                    pending = b''
                    while not self.stop.is_set():
                        pending += link.read(2048)
                        while b'\n' in pending:
                            raw, pending = pending.split(b'\n', 1)
                            try:
                                self.ingest(json.loads(raw))
                            except (ValueError, TypeError, KeyError):
                                pass
                        if len(pending) > 8192:
                            pending = b''
                        if time.monotonic() - max(opened, self.last_hello) > 4:
                            raise serial.SerialException('Firmware não respondeu')
            except (serial.SerialException, OSError) as exc:
                with self.lock:
                    if self.connected:
                        self.log('USB desconectada. O mapa salvo foi preservado.', 'warn')
                    self.connected = self.armed = False
                    self.live = None
                    self.link = None
                    self.message = 'Aguardando USB/UART: ' + str(exc)
                time.sleep(1)


def header_text(mapper):
    lines = ['#pragma once', '#include <stdint.h>', '// Generated HP48 contacts; -1 = GND, -2 = not mapped.',
             'struct KeyContact { const char *key; int8_t a, b; };', 'constexpr KeyContact hp48Keys[] = {']
    for key in KEYS:
        value = mapper.mapping.get(key['id']) or dict(a=-2, b=-2)
        lines.append('  {"%s", %d, %d},' % (key['id'], value['a'], value['b']))
    return '\n'.join(lines + ['};', 'constexpr unsigned hp48KeyCount = sizeof(hp48Keys)/sizeof(hp48Keys[0]);', ''])


def make_handler(mapper):
    token = secrets.token_urlsafe(24)

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def reply(self, content, mime='application/json; charset=utf-8', status=200, filename=None):
            if not isinstance(content, (str, bytes)):
                content = json.dumps(content, ensure_ascii=False)
            if isinstance(content, str):
                content = content.encode('utf-8')
            self.send_response(status)
            self.send_header('Content-Type', mime)
            self.send_header('Content-Length', str(len(content)))
            self.send_header('Cache-Control', 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            if filename:
                self.send_header('Content-Disposition', f'attachment; filename="{filename}"')
            self.end_headers()
            self.wfile.write(content)

        def valid_host(self):
            host = self.headers.get('Host', '')
            return host in (f'127.0.0.1:{self.server.server_port}', f'localhost:{self.server.server_port}')

        def do_GET(self):
            if not self.valid_host():
                return self.reply({'error': 'Host inválido'}, status=403)
            path = urlparse(self.path).path
            if path == '/':
                return self.reply((ROOT / 'index.html').read_text(encoding='utf-8').replace('__TOKEN__', token), 'text/html; charset=utf-8')
            if path == '/app.js':
                return self.reply((ROOT / 'app.js').read_bytes(), 'text/javascript; charset=utf-8')
            if path == '/api/state':
                return self.reply(mapper.state())
            with mapper.lock:
                if path == '/export/mapa.json':
                    return self.reply(mapper.document(), filename='hp48-mapa.json')
                if path == '/export/mapa.h':
                    return self.reply(header_text(mapper), 'text/plain; charset=utf-8', filename='hp48-mapa.h')
                if path == '/export/mapa.csv':
                    buf = io.StringIO()
                    writer = csv.writer(buf)
                    writer.writerow(['key', 'label', 'gpio_a', 'gpio_b', 'status'])
                    for key in KEYS:
                        val = mapper.mapping.get(key['id'])
                        writer.writerow([key['id'], key['label'], val['a'] if val else '', val['b'] if val else '', 'mapped' if val else 'pending'])
                    return self.reply('\ufeff' + buf.getvalue(), 'text/csv; charset=utf-8', filename='hp48-mapa.csv')
            return self.reply({'error': 'Não encontrado'}, status=404)

        def do_POST(self):
            if not self.valid_host() or self.headers.get('X-Mapper-Token') != token:
                return self.reply({'error': 'Origem inválida'}, status=403)
            if urlparse(self.path).path != '/api/action':
                return self.reply({'error': 'Não encontrado'}, status=404)
            try:
                length = int(self.headers.get('Content-Length', '0'))
                if not 0 < length < 4096:
                    raise ValueError('Requisição inválida')
                body = json.loads(self.rfile.read(length))
                mapper.action(body.get('action'), body.get('index'))
                self.reply(mapper.state())
            except (ValueError, OSError) as exc:
                self.reply({'error': str(exc)}, status=400)

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial-port')
    parser.add_argument('--http-port', type=int, default=8766)
    parser.add_argument('--open', action='store_true', help='Open the interface or reuse a running instance')
    args = parser.parse_args()
    url = f'http://127.0.0.1:{args.http_port}'
    if args.open:
        try:
            existing = json.load(urllib.request.urlopen(url + '/api/state', timeout=1))
            if existing.get('pins') == PINS and len(existing.get('keys', [])) == 49:
                webbrowser.open(url)
                print('A interface já está em execução.', flush=True)
                return
        except (OSError, ValueError):
            pass
    mapper = Mapper(args.serial_port)
    if not mapper.path.exists():
        mapper.save()
    server = ThreadingHTTPServer(('127.0.0.1', args.http_port), make_handler(mapper))
    threading.Thread(target=mapper.serial_loop, daemon=True).start()
    print(f'Interface: http://127.0.0.1:{args.http_port}', flush=True)
    if args.open:
        webbrowser.open(url)
    try:
        server.serve_forever()
    finally:
        mapper.stop.set()
        server.server_close()


if __name__ == '__main__':
    main()
