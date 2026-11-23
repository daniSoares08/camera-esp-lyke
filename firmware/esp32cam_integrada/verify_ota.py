"""Hardware integration test. Only bootstrapping is USB; update bytes use Wi-Fi.

Wi-Fi is bound to the requested interface; no Internet/API request is made.
The temporary profile is removed afterward. Optional radio restoration matches
the state observed at the start of this task.
"""
import argparse
import hashlib
from http.client import HTTPConnection, HTTPException
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import time
from xml.sax.saxutils import escape

import serial
from windows_wifi import set_radio

ROOT=Path(__file__).resolve().parents[2]
PROJECT='esp32cam_integrada'
INDEX=0

def local_ap_password():
    path=ROOT/'.private/ap-password.txt'
    password=os.environ.get('LYKE_LOCAL_AP_PSK') or (path.read_text(encoding='utf-8').strip() if path.exists() else '')
    if not password.isascii() or not 8<=len(password)<=63:
        raise RuntimeError('Configure .private/ap-password.txt or LYKE_LOCAL_AP_PSK')
    return password

def link(port):
    s=serial.Serial(port=None,baudrate=460800,timeout=.1,write_timeout=3)
    s.dtr=False;s.rts=False;s.port=port;s.open();return s

def command(s,text,prefix='OK ',timeout=8):
    s.reset_input_buffer();s.write((text+'\n').encode());deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        line=s.readline().decode('utf-8','replace').strip()
        if line.startswith(prefix):return line[len(prefix):]
        if line.startswith('ERR '):raise RuntimeError(line)
    raise TimeoutError('No response to '+text)

def status(s):return json.loads(command(s,'STATUS','INFO '))

def http(path,data=None,headers=None,timeout=8):
    sock=socket.socket(socket.AF_INET,socket.SOCK_STREAM);sock.settimeout(timeout)
    sock.setsockopt(socket.IPPROTO_IP,getattr(socket,'IP_UNICAST_IF',31),socket.htonl(INDEX))
    sock.connect(('192.168.8.1',80))
    conn=HTTPConnection('192.168.8.1',timeout=timeout);conn.sock=sock
    try:
        conn.request('GET' if data is None else 'POST',path,body=data,headers=headers or {})
        response=conn.getresponse();return response.status,response.read()
    finally:conn.close()

def multipart(path,filename,data,token,headers=None):
    boundary='ESPcamOtaBoundary2026'
    body=(f'--{boundary}\r\nContent-Disposition: form-data; name="firmware"; filename="{filename}"\r\nContent-Type: application/octet-stream\r\n\r\n').encode()+data+f'\r\n--{boundary}--\r\n'.encode()
    h={'Content-Type':'multipart/form-data; boundary='+boundary,'X-Update-Token':token}
    h.update(headers or {})
    return http(path,body,h,timeout=100)

def upload(data,token,filename=PROJECT+'.ino.bin',declared=None,sha=None):
    return multipart('/update',filename,data,token,{'X-Firmware-Size':str(len(data) if declared is None else declared),
                     'X-Firmware-SHA256':sha or hashlib.sha256(data).hexdigest()})

def connect(ssid):
    result=subprocess.run(['netsh','wlan','connect','name='+ssid],capture_output=True)
    if result.returncode:raise RuntimeError('Windows refused ESP Wi-Fi connection')
    deadline=time.monotonic()+35
    while time.monotonic()<deadline:
        try:
            code,raw=http('/info',timeout=2)
            info=json.loads(raw)
            if code==200 and info.get('project')==PROJECT:return info
        except (OSError,ValueError,HTTPException):pass
        time.sleep(1)
    raise TimeoutError('ESP AP not reachable over the selected Wi-Fi interface')

def main():
    global INDEX
    p=argparse.ArgumentParser()
    p.add_argument('--port',required=True);p.add_argument('--interface-index',required=True,type=int)
    p.add_argument('--binary',required=True,type=Path);p.add_argument('--report',required=True,type=Path)
    p.add_argument('--restore-radio-off',action='store_true');p.add_argument('--skip-storage',action='store_true')
    args=p.parse_args();INDEX=args.interface_index
    data=args.binary.read_bytes();expected_desc=data[32+16:32+48].split(b'\0',1)[0].decode()
    report={'transport':'HTTP over Wi-Fi; USB only for commands/telemetry','binary_sha256':hashlib.sha256(data).hexdigest(),
            'expected_revision':expected_desc,'checks':{}}
    ssid=None;profile=ROOT/'.private/esp32cam_pre_ota/test_wifi.xml'
    profile.parent.mkdir(parents=True,exist_ok=True)
    def check(name,value):
        report['checks'][name]=bool(value);print(name+': '+('PASS' if value else 'FAIL'),flush=True)
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        if not value:raise AssertionError(name)
    try:
        set_radio(True)
        with link(args.port) as s:
            initial=status(s);report['before']=initial
            check('classic_esp32_4mb',initial['chip']=='ESP32-D0WD-V3' and initial['flash_bytes']==4194304)
            check('two_1_5_mib_slots',initial['capacity']==1572864)
            check('psram_4mib',initial['psram_bytes']==4194304)
            check('missing_peripherals_tolerated',not initial['camera_ready'] and not initial['pcf_present'])
            check('calculator_works',command(s,'CALC 2+3*4','RESULT ')=='14')
            if not initial['fs_mounted']:command(s,'FORMAT')
            command(s,'CLEAR');ssid=command(s,'BENCH').split()[0]
            xml=f'''<?xml version="1.0"?><WLANProfile xmlns="http://www.microsoft.com/networking/WLAN/profile/v1"><name>{escape(ssid)}</name><SSIDConfig><SSID><name>{escape(ssid)}</name></SSID></SSIDConfig><connectionType>ESS</connectionType><connectionMode>manual</connectionMode><MSM><security><authEncryption><authentication>WPA2PSK</authentication><encryption>AES</encryption><useOneX>false</useOneX></authEncryption><sharedKey><keyType>passPhrase</keyType><protected>false</protected><keyMaterial>{escape(local_ap_password())}</keyMaterial></sharedKey></security></MSM></WLANProfile>'''
            profile.write_text(xml,encoding='utf-8')
            subprocess.run(['netsh','wlan','add','profile','filename='+str(profile),'user=current'],check=True,capture_output=True)
            info=connect(ssid);token=info['token'];check('http_status_matches_usb',info['revision']==initial['revision'] and info['slot']==initial['slot'])
            check('page_available',http('/')[0]==200)
            if not args.skip_storage:
                jpeg=(ROOT/'outputs/teste_esp32cam_2026-11-08/teste_af.jpg').read_bytes()
                for i in range(4):
                    code,_=multipart('/import','test.jpg',jpeg,token,{'X-Photo-Size':str(len(jpeg))})
                    check('jpeg_import_'+str(i+1),code==200)
                full=json.loads(http('/info')[1]);report['photo_capacity_test']={k:v for k,v in full.items() if k!='token'}
                check('three_ram_one_flash',len(full['photos'])==4 and full['ram_photo_bytes']==3*len(jpeg) and full['flash_photo_bytes']==len(jpeg))
                check('fifth_large_photo_refused',multipart('/import','test.jpg',jpeg,token,{'X-Photo-Size':str(len(jpeg))})[0]==400)
                for entry in full['photos']:
                    code,download=http('/photo?id='+str(entry['id']),timeout=30)
                    check('jpeg_download_'+str(entry['id']),code==200 and hashlib.sha256(download).digest()==hashlib.sha256(jpeg).digest())
                command(s,'REBOOT');time.sleep(4)
                recovered=status(s);check('flash_photo_persists_ram_clears',len(recovered['photos'])==1 and recovered['flash_photo_bytes']==len(jpeg) and recovered['ram_photo_bytes']==0)
                command(s,'BENCH');info=connect(ssid);token=info['token']
            # No update is allowed in gallery mode, even for a valid image.
            check('requires_update_mode',upload(data[:512],token,declared=len(data))[0]==400)
            check('enter_ota',http('/api/ota',b'',{'X-Update-Token':token})[0]==200)
            cleared=json.loads(http('/info')[1]);check('photos_erased_on_ota',len(cleared['photos'])==0 and cleared['ram_photo_bytes']==0 and cleared['flash_photo_bytes']==0)
            check('unauthorized_rejected',upload(data[:512],'invalid',declared=len(data))[0]==403)
            check('wrong_filename_rejected',upload(data[:512],token,filename='merged.bin',declared=len(data))[0]==400)
            chip=bytearray(data[:512]);chip[12:14]=(9).to_bytes(2,'little')
            check('wrong_chip_rejected',upload(chip,token,declared=len(data))[0]==400)
            project=bytearray(data[:512]);project[80:112]=b'wrong_project'.ljust(32,b'\0')
            check('wrong_project_rejected',upload(project,token,declared=len(data))[0]==400)
            check('oversize_rejected',upload(data[:512],token,declared=1572865)[0]==400)
            check('truncated_rejected',upload(data[:1024],token,declared=len(data))[0]==400)
            broken=bytearray(data);broken[-40]^=1
            check('corrupt_image_rejected',upload(broken,token)[0]==400)
            check('wrong_sha256_rejected',upload(data,token,sha='0'*64)[0]==400)
            unchanged=status(s);check('rejections_preserve_running_version',unchanged['slot']==initial['slot'] and unchanged['revision']==initial['revision'])
            started=time.monotonic();code,body=upload(data,token);report['upload_seconds']=round(time.monotonic()-started,2);report['upload_response']=body.decode(errors='replace')
            check('valid_wifi_upload_accepted',code==200)
            time.sleep(5);after=status(s);report['after']=after
            check('booted_other_slot',after['slot']!=initial['slot'])
            check('booted_requested_revision',after['revision']==expected_desc)
            check('booted_expected_image_digest',after['image_sha256']==data[-32:].hex())
            check('photos_remain_empty',not after['photos'])
            command(s,'WIFI');remote=connect(ssid)
            check('new_firmware_responds_over_wifi',remote['slot']==after['slot'] and remote['revision']==expected_desc)
            command(s,'OFF');check('wifi_can_stop',not status(s)['ota_active'])
        report['success']=True
    except Exception as exc:
        report['success']=False;report['error']=str(exc);raise
    finally:
        if ssid:
            subprocess.run(['netsh','wlan','disconnect'],capture_output=True)
            subprocess.run(['netsh','wlan','delete','profile','name='+ssid],capture_output=True)
        profile.unlink(missing_ok=True)
        if args.restore_radio_off:set_radio(False)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':main()
