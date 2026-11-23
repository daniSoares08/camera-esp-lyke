from pathlib import Path

root = Path(__file__).resolve().parent
page = (root / 'esquema-stm32.template.html').read_text(encoding='utf-8')
plan = (root / 'stm32-esp32cam.json').read_text(encoding='utf-8')
assert page.count('__PLAN__') == 1
page = page.replace('__PLAN__', plan)
destination = root / 'ESP-CAM-esquema.html'
destination.write_text(page, encoding='utf-8')
print(destination)
