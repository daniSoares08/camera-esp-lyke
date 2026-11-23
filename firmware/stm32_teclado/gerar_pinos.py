"""Gera a tabela do firmware a partir do mesmo mapa exibido no HTML."""

import json
import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[2]
plan = json.loads((root / "outputs/ligacoes/stm32-esp32cam.json").read_text(encoding="utf-8"))
tracks = [pin for group in plan["keyboard"] for pin in group["pins"]]
lcd = {link["signal"]: link["stm"] for link in plan["lcd"]}
functions = {"CS": "PA4", "RSE": "PA6", "RS": "PA1", "SCL": "PA5", "SI": "PA7"}
if len(tracks) != 19 or tracks[-1] != "PC6" or any(lcd[key] != pin for key, pin in functions.items()):
    raise SystemExit("Mapa incompatível com a bancada STM32 R1.")
used = tracks + list(functions.values()) + [link["stm"] for link in plan["uart"]] + [plan["battery"]["stm"]]
if len(set(used)) != len(used) or set(used) & {"PA11", "PA12", "PA13", "PA14", "PB8"}:
    raise SystemExit("GPIO repetido ou reservado no mapa.")


def pin_initializer(name):
    match = re.fullmatch(r"P([ABC])(1[0-5]|[0-9])", name)
    if match is None:
        raise ValueError(f"GPIO inválido: {name}")
    return f'{{ GPIO{match[1]}, {match[2]}, "{name:3}" }}'


lines = [
    "// Gerado a partir de stm32-esp32cam.json.",
    "typedef struct { GPIO_TypeDef *port; uint8_t bit; char label[4]; } Pin;",
    "static const Pin tracks[TRACK_COUNT] = {",
    *(f"    {pin_initializer(pin)}," for pin in tracks),
    "};",
    *(f"static const Pin lcd_{key.lower()} = {pin_initializer(lcd[key])};" for key in functions),
]
Path(sys.argv[1]).write_text("\n".join(lines) + "\n", encoding="ascii")
