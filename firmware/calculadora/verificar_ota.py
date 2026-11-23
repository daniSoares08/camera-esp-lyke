"""Exercise real Wi-Fi OTA, restoring the PC Wi-Fi profile in all cases.

Uses USB only to open the temporary AP and read post-boot state; the firmware
itself is sent over HTTP/Wi-Fi. Does not read stored Wi-Fi/API credentials.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import time
import urllib.error
import urllib.request
import http.client
import socket
from xml.sax.saxutils import escape

import serial
from configurar_usb import Decoder

ROOT = Path(__file__).resolve().parents[2]
BASE_URL='http://192.168.8.1'
INTERFACE_INDEX=0


class InterfaceHTTPConnection(http.client.HTTPConnection):
    def connect(self):
        self.sock=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        self.sock.settimeout(self.timeout)
        if INTERFACE_INDEX:
            self.sock.setsockopt(socket.IPPROTO_IP,getattr(socket,'IP_UNICAST_IF',31),socket.htonl(INTERFACE_INDEX))
        self.sock.connect((self.host,self.port))


class InterfaceHTTPHandler(urllib.request.HTTPHandler):
    def http_open(self,req):
        return self.do_open(InterfaceHTTPConnection,req)


HTTP = urllib.request.build_opener(urllib.request.ProxyHandler({}),InterfaceHTTPHandler())


def open_serial(port):
    s = serial.Serial(port=None, baudrate=115200, timeout=.05, write_timeout=2)
    s.dtr = False
    s.rts = False
    s.port = port
    s.open()
    return s


def request(path, data=None, headers=None):
    r = urllib.request.Request(BASE_URL+path, data=data,
                               headers=headers or {})
    try:
        with HTTP.open(r, timeout=60 if data is not None else 2) as response:
            return response.status, response.read().decode('utf8', 'replace')
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode('utf8', 'replace')


def upload(filename, data, token, md5=None):
    boundary = 'CalcFirmwareBoundary2026'
    body = (f'--{boundary}\r\nContent-Disposition: form-data; name="firmware"; '
            f'filename="{filename}"\r\nContent-Type: application/octet-stream\r\n\r\n').encode()
    body += data + f'\r\n--{boundary}--\r\n'.encode()
    headers = {'Content-Type': 'multipart/form-data; boundary='+boundary,
               'X-Update-Token': token}
    if md5:
        headers['X-Firmware-MD5'] = md5
    return request('/update', body, headers)


def command(link, body):
    d = Decoder()
    body['id'] = 'ota-test'
    link.write((json.dumps(body)+'\n').encode())
    end = time.monotonic()+5
    while time.monotonic() < end:
        for kind, value in d.feed(link.read(4096)):
            if kind == 'ack' and value.get('id') == 'ota-test':
                return value
    raise RuntimeError('No USB acknowledgement')


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--target', choices=['s3', 'display'], required=True)
    p.add_argument('--restore-profile', required=True)
    args = p.parse_args()
    target = 's3_calculadora' if args.target == 's3' else 'devkit_display'
    other = 'devkit_display' if args.target == 's3' else 's3_calculadora'
    binary = (ROOT/'firmware/calculadora/bin'/target/(target+'.ino.bin')).read_bytes()
    other_binary = (ROOT/'firmware/calculadora/bin'/other/(other+'.ino.bin')).read_bytes()
    result = {'target': target, 'transport': 'HTTP over Wi-Fi', 'checks': {}}
    temporary = ROOT/'.build/calculadora/ota_wifi_profile.xml'
    ssid = None
    with open_serial('COM12') as s3:
        setup = command(s3, {'cmd': 'ota', 'target': args.target})['setup_text']
        password = re.search(r'Senha: ([^\n]+)', setup).group(1).strip() if args.target=='s3' else 'CHANGE_ME_ESP_AP'
        ssid = setup.splitlines()[0] if args.target == 's3' else 'CalcTela-1d44'
        profile = f'''<?xml version="1.0"?><WLANProfile xmlns="http://www.microsoft.com/networking/WLAN/profile/v1"><name>{escape(ssid)}</name><SSIDConfig><SSID><name>{escape(ssid)}</name></SSID></SSIDConfig><connectionType>ESS</connectionType><connectionMode>manual</connectionMode><MSM><security><authEncryption><authentication>WPA2PSK</authentication><encryption>AES</encryption><useOneX>false</useOneX></authEncryption><sharedKey><keyType>passPhrase</keyType><protected>false</protected><keyMaterial>{password}</keyMaterial></sharedKey></security></MSM></WLANProfile>'''
        temporary.write_text(profile, encoding='utf8')
        try:
            add = subprocess.run(['netsh', 'wlan', 'add', 'profile', f'filename={temporary}', 'user=current'], capture_output=True)
            connect = subprocess.run(['netsh', 'wlan', 'connect', f'name={ssid}'], capture_output=True)
            if add.returncode or connect.returncode:
                raise RuntimeError('Windows refused Wi-Fi profile/connection: '+connect.stdout.decode(errors='replace'))
            info = None
            for _ in range(12):
                try:
                    code, body = request('/info')
                    if code == 200:
                        candidate = json.loads(body)
                        if candidate['project'] == target:
                            info = candidate
                            break
                except (OSError, ValueError):
                    pass
                time.sleep(1)
            if info is None:
                raise RuntimeError('Cannot reach the temporary Wi-Fi AP')
            result['before_slot'] = info['slot']
            token = info['token']
            result['checks']['unauthorized_rejected'] = upload(target+'.ino.bin', b'bad', 'wrong')[0] == 403
            result['checks']['wrong_filename_rejected'] = upload('merged.bin', binary[:1024], token)[0] == 400
            result['checks']['wrong_chip_rejected'] = upload(target+'.ino.bin', other_binary[:1024], token)[0] == 400
            damaged = bytearray(binary)
            damaged[-40] ^= 1
            result['checks']['corrupt_image_rejected'] = upload(target+'.ino.bin', damaged, token, hashlib.md5(binary).hexdigest())[0] == 400
            if not all(result['checks'].values()):
                raise RuntimeError('A rejection check failed: '+str(result['checks']))
            code, body = upload(target+'.ino.bin', binary, token, hashlib.md5(binary).hexdigest())
            result['checks']['valid_upload_accepted'] = code == 200
            if code != 200:
                raise RuntimeError(f'OTA rejected ({code}): {body}')
            time.sleep(5)
            if args.target == 's3':
                s3.close()
                with open_serial('COM12') as fresh:
                    status = command(fresh, {'cmd': 'status'})
                result['after_slot'] = status['ota_slot']
                result['api_key_still_set'] = status['api_key_set']
                result['checks']['booted_other_slot'] = result['after_slot'] != result['before_slot']
            else:
                with open_serial('COM14') as devkit:
                    devkit.write(b's')
                    end = time.monotonic()+3
                    text = bytearray()
                    while time.monotonic()<end:
                        text.extend(devkit.read(2048))
                    status = text.decode('utf8', 'replace')
                    result['after_slot'] = re.search(r'slot=(\w+)', status).group(1)
                    result['checks']['booted_other_slot'] = result['after_slot'] != result['before_slot']
            if not result['checks']['booted_other_slot']:
                raise RuntimeError('New OTA slot did not boot')
        finally:
            subprocess.run(['netsh', 'wlan', 'connect', 'name='+args.restore_profile], capture_output=True)
            temporary.unlink(missing_ok=True)
            subprocess.run(['netsh', 'wlan', 'delete', 'profile', 'name='+ssid], capture_output=True)
            time.sleep(4)
    report = ROOT/'outputs/calculadora'/('ota_'+args.target+'_verificacao.json')
    report.write_text(json.dumps(result, indent=2)+'\n', encoding='utf8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
