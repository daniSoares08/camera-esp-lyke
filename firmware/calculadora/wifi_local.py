"""Connect to the calculator AP; upload only through Wi-Fi (no serial port)."""
import argparse
import json
from pathlib import Path
import subprocess
import time
from xml.sax.saxutils import escape
from verificar_ota import request, upload
import verificar_ota
import hashlib

ROOT=Path(__file__).resolve().parents[2]


def connect(ssid,password):
    profile=f'''<?xml version="1.0"?><WLANProfile xmlns="http://www.microsoft.com/networking/WLAN/profile/v1"><name>{escape(ssid)}</name><SSIDConfig><SSID><name>{escape(ssid)}</name></SSID></SSIDConfig><connectionType>ESS</connectionType><connectionMode>manual</connectionMode><MSM><security><authEncryption><authentication>WPA2PSK</authentication><encryption>AES</encryption><useOneX>false</useOneX></authEncryption><sharedKey><keyType>passPhrase</keyType><protected>false</protected><keyMaterial>{escape(password)}</keyMaterial></sharedKey></security></MSM></WLANProfile>'''
    path=ROOT/'.build/calculadora/wifi_local_profile.xml';path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(profile,encoding='utf8')
    try:
        for cmd in (['netsh','wlan','add','profile',f'filename={path}','user=current'],['netsh','wlan','connect',f'name={ssid}']):
            result=subprocess.run(cmd,capture_output=True)
            if result.returncode:raise RuntimeError(result.stdout.decode(errors='replace'))
    finally:path.unlink(missing_ok=True)


def wait(path='/info',timeout=25):
    end=time.monotonic()+timeout
    while time.monotonic()<end:
        try:
            status,body=request(path)
            if status==200:return body
        except (OSError,ValueError):pass
        time.sleep(.5)
    raise RuntimeError('A rede local nao respondeu. Confira se o modo correspondente continua ativo no LCD.')


def main():
    p=argparse.ArgumentParser();p.add_argument('--ssid',required=True);p.add_argument('--password',required=True);p.add_argument('--upload');p.add_argument('--path',default='/info');p.add_argument('--host',default='192.168.8.1');p.add_argument('--interface',type=int,default=0);args=p.parse_args()
    verificar_ota.BASE_URL='http://'+args.host;verificar_ota.INTERFACE_INDEX=args.interface
    connect(args.ssid,args.password);body=wait(args.path)
    if not args.upload:
        print(body);return
    info=json.loads(body);binary=Path(args.upload);expected=info['project']+'.ino.bin'
    if binary.name!=expected:raise RuntimeError('O arquivo nao corresponde ao destino')
    data=binary.read_bytes();status,response=upload(binary.name,data,info['token'],hashlib.md5(data).hexdigest())
    if status!=200:raise RuntimeError(f'OTA: {status}: {response}')
    report={'ssid':args.ssid,'target':info['project'],'before_slot':info['slot'],'http':status,'response':response,'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'transport':'Wi-Fi only; no USB'}
    (ROOT/'outputs/calculadora'/('wifi_upload_'+info['project']+'.json')).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
