#!/bin/sh
set -eu

here=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH='' cd -- "$here/../.." && pwd)
sdk="$root/.tools/stm32"
revision=${1:-lyke-r13}
case "$revision" in
    ''|[!A-Za-z0-9]*|*[!A-Za-z0-9._-]*) echo "Invalid revision" >&2; exit 1 ;;
esac
out="$root/.build/lyke_stm32/$revision"
mkdir -p "$out"
python3 "$root/firmware/stm32_teclado/gerar_pinos.py" "$out/pins.h"

compile() {
    name=$1 origin=$2 limit=$3
    shift 3
    extra_sources=""
    if [ "$name" = boot ]; then extra_sources="$here/vendor/puff.c"; fi
    arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -std=c11 -Oz -flto -g -ffreestanding "-DLYKE_REVISION=\"$revision\"" \
        -ffunction-sections -fdata-sections -Wall -Wextra -Werror \
        -I "$sdk/cmsis-device-g4/Include" -I "$sdk/CMSIS_5/CMSIS/Core/Include" -I "$out" \
        "$here/$name.c" $extra_sources "$sdk/cmsis-device-g4/Source/Templates/gcc/startup_stm32g431xx.s" \
        -Wl,--defsym,LYKE_ORIGIN="$origin",--defsym,LYKE_LENGTH="$limit" \
        -T "$here/lyke.ld" -nostartfiles -Wl,--gc-sections,-Map,"$out/$name.map" \
        "$@" -o "$out/$name.elf"
    arm-none-eabi-objcopy -O binary "$out/$name.elf" "$out/$name.bin"
    arm-none-eabi-size "$out/$name.elf"
}

compile boot 0x08000000 0x2000 -nostdlib -lc_nano -lgcc
compile main 0x08002000 0x10000 --specs=nano.specs --specs=nosys.specs -lm
python3 "$here/pack.py" "$out" "$revision"
