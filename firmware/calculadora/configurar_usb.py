"""Private USB setup and live framebuffer viewer for the calculator S3."""
import binascii
import json
import queue
import secrets
import threading
import time
import tkinter as tk
from tkinter import ttk
import webbrowser

import serial
from serial.tools import list_ports
from PIL import Image, ImageTk


class Decoder:
    def __init__(self):
        self.buffer = bytearray()
        self.control = bytearray()

    def feed(self, chunk):
        self.control.extend(chunk)
        self.control = self.control[-16384:]
        messages = []
        while True:
            start = self.control.find(b'\nCFG ')
            if start < 0:
                break
            end = self.control.find(b'\n', start + 5)
            if end < 0:
                break
            try:
                messages.append(('ack', json.loads(self.control[start + 5:end])))
            except (ValueError, UnicodeError):
                pass
            del self.control[:end + 1]
        self.buffer.extend(chunk)
        while True:
            at = self.buffer.find(b'\xc3\x3c')
            if at < 0:
                self.buffer = self.buffer[-1:]
                break
            if at:
                del self.buffer[:at]
            if len(self.buffer) < 10:
                break
            if self.buffer[2:4] != b'\x01\x01' or self.buffer[6:8] != b'\x00\x04':
                del self.buffer[0]
                continue
            if len(self.buffer) < 1034:
                break
            packet = self.buffer[:1034]
            crc = binascii.crc_hqx(packet[2:8] + packet[10:], 0xFFFF)
            if crc == int.from_bytes(packet[8:10], 'little'):
                messages.append(('frame', bytes(packet[10:])))
                del self.buffer[:1034]
            else:
                del self.buffer[0]
        return messages


class SetupApp:
    def __init__(self, root):
        self.root = root
        root.title('Configurar calculadora · USB/UART')
        root.geometry('690x960')
        self.link = None
        self.events = queue.Queue()
        self.write_lock = threading.Lock()
        self.pending = set()
        self.port = tk.StringVar()
        self.key = tk.StringVar()
        self.ssid = tk.StringVar()
        self.password = tk.StringVar()
        self.model = tk.StringVar(value='Preciso — GPT-6.1 Sol')
        self.status = tk.StringVar(value='Conecte o S3 pelo conector USB/UART e escolha a porta.')
        self.info = tk.StringVar(value='A chave será enviada diretamente ao ESP; não será salva neste computador.')
        frame = ttk.Frame(root, padding=22)
        frame.pack(fill='both', expand=True)
        ttk.Label(frame, text='Configurar calculadora', font=('Segoe UI', 20, 'bold')).pack(anchor='w')
        ttk.Label(frame, text='ESP32-S3 · USB/UART · 115200 baud').pack(anchor='w', pady=(0, 16))
        ports = ttk.Frame(frame)
        ports.pack(fill='x')
        self.combo = ttk.Combobox(ports, textvariable=self.port, state='readonly', width=42)
        self.combo.pack(side='left', fill='x', expand=True)
        ttk.Button(ports, text='Atualizar', command=self.refresh).pack(side='left', padx=5)
        ttk.Button(ports, text='Conectar', command=self.connect).pack(side='left')
        ttk.Label(frame, textvariable=self.status, wraplength=620).pack(anchor='w', pady=12)
        ttk.Label(frame, text='Chave OpenAI API (deixe vazio para manter a atual)').pack(anchor='w')
        ttk.Entry(frame, textvariable=self.key, show='•').pack(fill='x', pady=(3, 10))
        ttk.Label(frame, text='Modelo das consultas por foto').pack(anchor='w')
        ttk.Combobox(frame, textvariable=self.model, values=['Preciso — GPT-6.1 Sol', 'Econômico — GPT-6 Luna'], state='readonly').pack(fill='x', pady=(3, 10))
        ttk.Label(frame, text='Wi-Fi adicional / atualizar senha (opcional)').pack(anchor='w')
        ttk.Entry(frame, textvariable=self.ssid).pack(fill='x', pady=3)
        ttk.Entry(frame, textvariable=self.password, show='•').pack(fill='x', pady=3)
        ttk.Button(frame, text='Salvar configuração no ESP', command=self.save).pack(fill='x', pady=10)
        ttk.Label(frame, textvariable=self.info, wraplength=620).pack(anchor='w')
        links = ttk.Frame(frame)
        links.pack(fill='x', pady=10)
        ttk.Button(links, text='Gerenciar chaves OpenAI', command=lambda: webbrowser.open('https://platform.openai.com/api-keys')).pack(side='left')
        ttk.Button(links, text='Créditos e cobrança', command=lambda: webbrowser.open('https://platform.openai.com/settings/organization/billing/overview')).pack(side='left', padx=7)
        ttk.Label(frame, text='Tela enviada pelo S3', font=('Segoe UI', 12, 'bold')).pack(anchor='w', pady=(12, 4))
        self.photo = ImageTk.PhotoImage(Image.new('RGB', (512, 256), '#c9df88'))
        self.display = tk.Label(frame, bg='#303c22', image=self.photo)
        self.display.pack(fill='x')
        controls = ttk.Frame(frame)
        controls.pack(fill='x', pady=10)
        ttk.Button(controls, text='Abrir menu (E)', command=self.open_menu).pack(side='left')
        ttk.Button(controls, text='ON / Voltar', command=lambda: self.send(dict(cmd='key', key='ON', down=True))).pack(side='left', padx=7)
        ttk.Button(controls, text='Ler estado', command=lambda: self.send(dict(cmd='status'))).pack(side='left')
        self.refresh()
        root.after(100, self.poll)
        root.protocol('WM_DELETE_WINDOW', self.close)

    def refresh(self):
        values = [f'{p.device} · {p.description}' for p in list_ports.comports()]
        self.combo['values'] = values
        if values and self.port.get() not in values:
            self.port.set(values[0])

    def connect(self):
        name = self.port.get().split(' · ')[0]
        if not name:
            self.status.set('Nenhuma porta selecionada.')
            return
        if self.link:
            self.link.close()
        try:
            link = serial.Serial(port=None, baudrate=115200, timeout=.1, write_timeout=2)
            link.dtr = False
            link.rts = False
            link.port = name
            link.open()
            self.link = link
            threading.Thread(target=self.read_loop, args=(link,), daemon=True).start()
            self.status.set('Conectado. Aguardando identificação do firmware…')
            self.root.after(1000, lambda: self.send(dict(cmd='status')))
        except (serial.SerialException, OSError):
            self.status.set('Não foi possível abrir a porta. Feche o mapeador ou outro monitor serial.')

    def read_loop(self, link):
        decoder = Decoder()
        try:
            while link.is_open:
                for message in decoder.feed(link.read(4096)):
                    self.events.put(message)
        except (serial.SerialException, OSError):
            self.events.put(('error', 'A conexão USB foi interrompida.'))

    def send(self, body):
        if not self.link or not self.link.is_open:
            self.status.set('Conecte o ESP antes de continuar.')
            return
        body['id'] = secrets.token_hex(8)
        self.pending.add(body['id'])
        try:
            with self.write_lock:
                self.link.write((json.dumps(body, ensure_ascii=True) + '\n').encode())
        except (serial.SerialException, OSError):
            self.status.set('Falha ao enviar a configuração por USB.')

    def save(self):
        key = self.key.get().strip()
        if key and (not key.startswith('sk-') or not 20 <= len(key) <= 240):
            self.status.set('Use uma chave própria da API, começando com sk-.')
            return
        self.send(dict(cmd='configure', key=key, ssid=self.ssid.get(), password=self.password.get(), economy=self.model.get().startswith('Econômico')))
        self.key.set('')
        self.password.set('')

    def open_menu(self):
        self.send(dict(cmd='key', key='E', down=True))

    def poll(self):
        latest = None
        while True:
            try:
                kind, value = self.events.get_nowait()
            except queue.Empty:
                break
            if kind == 'frame':
                latest = value
            elif kind == 'error':
                self.status.set(value)
            elif value.get('id') in self.pending:
                self.pending.discard(value['id'])
                if 'firmware' in value:
                    self.status.set(f"{value['firmware']} · chave {'configurada' if value.get('api_key_set') else 'ausente'}")
                    self.info.set('Redes salvas: ' + ', '.join(value.get('profiles', [])) + '. Câmera: ' + ('interface ativa' if value.get('camera_interface_active') else 'interface parada'))
                    self.model.set('Econômico — GPT-6 Luna' if value.get('economy') else 'Preciso — GPT-6.1 Sol')
                else:
                    self.status.set(value.get('message', 'Comando recebido.' if value.get('ok') else 'Comando recusado.'))
        if latest is not None:
            image = Image.new('RGB', (128, 64), '#c9df88')
            for y in range(64):
                for x in range(128):
                    if latest[(y // 8) * 128 + x] & (1 << (y % 8)):
                        image.putpixel((x, y), (28, 45, 23))
            self.photo = ImageTk.PhotoImage(image.resize((512, 256), Image.Resampling.NEAREST))
            self.display.configure(image=self.photo, height=256)
        self.root.after(100, self.poll)

    def close(self):
        if self.link:
            self.link.close()
        self.root.destroy()


if __name__ == '__main__':
    window = tk.Tk()
    SetupApp(window)
    window.mainloop()
