#!/bin/sh
set -eu

firmware_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
project=$(CDPATH='' cd -- "$firmware_dir/../.." && pwd)
sdk="$project/.tools/stm32"
out="$project/.build/stm32_teclado"
mkdir -p "$out"
python3 "$firmware_dir/gerar_pinos.py" "$out/pins.h"

arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -std=c11 -Os -g -ffreestanding \
    -ffunction-sections -fdata-sections -Wall -Wextra -Werror \
    -I "$sdk/cmsis-device-g4/Include" -I "$sdk/CMSIS_5/CMSIS/Core/Include" -I "$out" \
    "$firmware_dir/main.c" "$sdk/cmsis-device-g4/Source/Templates/gcc/startup_stm32g431xx.s" \
    -T "$firmware_dir/stm32g431cb.ld" -nostdlib -Wl,--gc-sections,-Map,"$out/mapeador.map" \
    -lgcc -o "$out/mapeador.elf"
arm-none-eabi-objcopy -O binary "$out/mapeador.elf" "$out/mapeador.bin"
arm-none-eabi-size "$out/mapeador.elf"
