"""Grava via TinyUSB/OTG, acompanhando a mudança de porta no bootloader."""
import argparse
from pathlib import Path
import subprocess
import sys
import time
import serial
from serial.tools import list_ports
from esptool.reset import ClassicReset


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True)
    args = parser.parse_args()
    binary = Path(__file__).resolve().parent / 'release_pc' / 'serial_camera_usb.ino.bin'
    if not binary.exists() or not 0 < binary.stat().st_size < 0x140000:
        raise RuntimeError('Compile o firmware primeiro; binário deve caber na partição app0.')
    devices = {p.device: p for p in list_ports.comports()}
    device = devices.get(args.port)
    if device is None or device.vid != 0x303A:
        raise RuntimeError('A porta informada não corresponde ao ESP conectado por OTG.')
    identity = (device.serial_number or '').replace(':', '').upper()
    with serial.Serial(args.port, 115200, timeout=.2) as connection:
        time.sleep(.3)
        connection.reset_input_buffer()
        connection.write(b'I\n')
        deadline = time.monotonic()+2
        while time.monotonic() < deadline:
            if connection.readline().startswith(b'INFO '):
                # TinyUSB uses the classic sequence even with VID:PID 303A:1001.
                connection.dtr = False
                connection.rts = False
                time.sleep(.2)
                try:
                    ClassicReset(connection)()
                except serial.SerialException:
                    pass  # Device may disappear during the expected USB reenumeration.
                break
    time.sleep(2)
    matches = [p for p in list_ports.comports() if p.vid == 0x303A and
               (p.serial_number or '').replace(':', '').upper() == identity]
    if len(matches) != 1:
        raise RuntimeError('Bootloader não encontrado. Segure BOOT, toque RESET e tente novamente.')
    port = matches[0].device
    print(f'Gravando aplicação em {port}; bootloader, partições e NVS preservados.', flush=True)
    result = subprocess.run([sys.executable, '-m', 'esptool', '--chip', 'esp32s3',
                             '--port', port, '--baud', '460800', '--before', 'no-reset',
                             '--after', 'hard-reset', 'write-flash', '0x10000', str(binary)])
    raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
