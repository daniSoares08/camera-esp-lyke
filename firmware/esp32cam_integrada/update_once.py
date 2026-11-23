"""Send one application over Wi-Fi; confirm revision, slot and image digest."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
from xml.sax.saxutils import escape

import verify_ota as protocol
from windows_wifi import set_radio

def main():
    p=argparse.ArgumentParser();p.add_argument('--port',required=True);p.add_argument('--binary',type=Path,required=True)
    p.add_argument('--interface-index',type=int,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--restore-radio-off',action='store_true')
    p.add_argument('--keep-portal',action='store_true')
    args=p.parse_args()
    protocol.INDEX=args.interface_index
    data=args.binary.read_bytes();manifest=json.loads((args.binary.parent/'manifest.json').read_text())
    expected=manifest['files'][args.binary.name]
    if len(data)!=expected['bytes'] or hashlib.sha256(data).hexdigest()!=expected['sha256']:
        raise RuntimeError('Binary does not match release manifest')
    if manifest['chip']!='esp32' or len(data)>1572864:raise RuntimeError('Wrong target/oversize image')
    report={'success':False,'transport':'HTTP over Wi-Fi','binary_sha256':expected['sha256']}
    profile=protocol.ROOT/'.private/esp32cam_pre_ota/one_update_wifi.xml';ssid=None
    try:
        with protocol.link(args.port) as s:
            before=protocol.status(s);report['before']=before
            if before['project']!=protocol.PROJECT or before['flash_bytes']!=4194304 or before['chip']!='ESP32-D0WD-V3':
                raise RuntimeError('Connected target mismatch')
            ssid=protocol.command(s,'OTA').split()[0]
            profile.parent.mkdir(parents=True,exist_ok=True)
            profile.write_text(f'''<?xml version="1.0"?><WLANProfile xmlns="http://www.microsoft.com/networking/WLAN/profile/v1"><name>{escape(ssid)}</name><SSIDConfig><SSID><name>{escape(ssid)}</name></SSID></SSIDConfig><connectionType>ESS</connectionType><connectionMode>manual</connectionMode><MSM><security><authEncryption><authentication>WPA2PSK</authentication><encryption>AES</encryption><useOneX>false</useOneX></authEncryption><sharedKey><keyType>passPhrase</keyType><protected>false</protected><keyMaterial>{escape(protocol.local_ap_password())}</keyMaterial></sharedKey></security></MSM></WLANProfile>''',encoding='utf-8')
            set_radio(True)
            subprocess.run(['netsh','wlan','add','profile','filename='+str(profile),'user=current'],capture_output=True,check=True)
            info=protocol.connect(ssid);started=time.monotonic();code,body=protocol.upload(data,info['token'])
            report['upload_seconds']=round(time.monotonic()-started,2)
            if code!=200:raise RuntimeError('OTA refused: '+body.decode(errors='replace'))
            time.sleep(5);after=protocol.status(s);report['after']=after
            if after['slot']==before['slot'] or after['revision']!=manifest['revision'] or after['image_sha256']!=data[-32:].hex():
                raise RuntimeError('Post-boot revision/slot/digest mismatch')
            if not args.keep_portal:protocol.command(s,'OFF')
            report['success']=True
    except Exception as exc:
        report['error']=str(exc);raise
    finally:
        if ssid:
            subprocess.run(['netsh','wlan','disconnect'],capture_output=True)
            subprocess.run(['netsh','wlan','delete','profile','name='+ssid],capture_output=True)
        profile.unlink(missing_ok=True)
        if args.restore_radio_off:set_radio(False)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'success':True,'revision':report['after']['revision'],'slot':report['after']['slot'],'seconds':report['upload_seconds']}))

if __name__=='__main__':main()
