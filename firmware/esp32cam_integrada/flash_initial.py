"""Initial USB installation: preserve NVS; explicitly replace the app/OTA layout."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from serial.tools import list_ports

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent

def run(*args):subprocess.run([sys.executable,'-m','esptool',*map(str,args)],check=True,cwd=ROOT)

def main():
    p=argparse.ArgumentParser();p.add_argument('--port',required=True);p.add_argument('--revision',required=True);args=p.parse_args()
    port=next((x for x in list_ports.comports() if x.device==args.port),None)
    if port is None or (port.vid,port.pid,port.serial_number)!=(0x0403,0x6001,'A5069RR4A'):
        raise RuntimeError('The previously identified ESP32-CAM FTDI interface is not connected to this port')
    backup=ROOT/'.private/esp32cam_pre_ota/boot-particoes-nvs.bin'
    if not backup.exists() or backup.stat().st_size!=65536:
        raise RuntimeError('A complete 64 KiB boot/NVS backup is required before the initial migration')
    folder=HERE/'bin'/args.revision;manifest=json.loads((folder/'manifest.json').read_text())
    for name,info in manifest['files'].items():
        data=(folder/name).read_bytes()
        if len(data)!=info['bytes'] or hashlib.sha256(data).hexdigest()!=info['sha256']:
            raise RuntimeError('Invalid release hash: '+name)
    if manifest['chip']!='esp32' or manifest['flash_bytes']!=4194304:raise RuntimeError('Wrong chip/flash manifest')
    opts=['--chip','esp32','--port',args.port,'--baud','115200']
    run(*opts,'flash-id')
    # Refuse a different installed application rather than inferring the target
    # from an old COM number or an image of the board.
    run(*opts,'verify-flash','0x10000',ROOT/'firmware/release_esp32cam/serial_camera_usb.ino.bin')
    run(*opts,'write-flash','--flash-mode','dio','--flash-freq','80m','--flash-size','4MB',
        '0x1000',folder/'esp32cam_integrada.ino.bootloader.bin',
        '0x8000',folder/'esp32cam_integrada.ino.partitions.bin',
        '0xe000',folder/'boot_app0.bin','0x10000',folder/'esp32cam_integrada.ino.bin')
    run(*opts,'verify-flash','0x8000',folder/'esp32cam_integrada.ino.partitions.bin',
        '0x10000',folder/'esp32cam_integrada.ino.bin')

if __name__=='__main__':main()
