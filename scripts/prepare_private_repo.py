"""Prepare a credential-free source/document snapshot for GitHub, including public repos.

Original workspace, captures, NVS and firmware binaries are kept locally.
The publication copy never includes these private artifacts.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]
TREES = ('firmware', 'docs', 'outputs/ligacoes', 'teclado', 'supabase', 'scripts')
SKIP_PARTS = {'bin', 'build', '__pycache__', 'node_modules', '.git', '.temp', '.branches'}
EXTENSIONS = {'.ino', '.h', '.c', '.cpp', '.py', '.ps1', '.sh', '.ld', '.md', '.json', '.csv',
              '.html', '.svg', '.png', '.jpg', '.jpeg', '.txt', '.cjs', '.js', '.ts', '.toml', '.sql', '.css', '.bat'}
ROOT_FILES = ('README.md', '.gitignore', '.gitattributes', 'AGENTS.md', 'LEIA-ME-MIGRACAO.md', 'requirements.txt',
              'Abrir camera.bat', 'Abrir mapeador do teclado.bat', 'Configurar calculadora.bat', 'Instalar ferramentas.ps1',
              'docs/manual/Manual-Lyke.pdf')
TOKEN_PATTERN = re.compile(r'(?:sk-[A-Za-z0-9_-]{24,}|sbp_[A-Za-z0-9_-]{24,}|sb_secret_[A-Za-z0-9_-]{20,}|gh[pousr]_[A-Za-z0-9]{20,})')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT/'.private/github_repo')
    args = parser.parse_args()
    output = args.output.resolve()
    if ROOT not in output.parents or '.private' not in output.relative_to(ROOT).parts:
        raise SystemExit('The publication staging directory must be inside this workspace .private directory')
    output.mkdir(parents=True, exist_ok=True)
    candidates = [ROOT/name for name in ROOT_FILES if (ROOT/name).is_file()]
    for name in TREES:
        for path in (ROOT/name).rglob('*'):
            if (path.is_file() and not any(part in SKIP_PARTS for part in path.relative_to(ROOT).parts)
                    and path.suffix.lower() in EXTENSIONS and not path.name.endswith('.local.h')
                    and not ('secret' in path.name.lower() and 'example' not in path.name.lower())):
                candidates.append(path)
    # Values are loaded only from local files and never printed or published.
    private_values = []
    ap_file = ROOT/'.private/ap-password.txt'
    if ap_file.exists():
        private_values.append((ap_file.read_text().strip(), 'CHANGE_ME_ESP_AP'))
    wifi_file = ROOT/'firmware/calculadora/secrets.local.h'
    if wifi_file.exists():
        for value in re.findall(r'#define\s+\w*PASSWORD\s+"([^"\r\n]+)"', wifi_file.read_text(encoding='utf-8-sig')):
            private_values.append((value, 'CONFIGURE_LOCAL_WIFI_PASSWORD'))
    for name in ['device-token.txt','openai-key.txt','supabase-access-token.txt']:
        local=ROOT/'.private/lyke-cloud'/name
        if local.exists():private_values.append((local.read_text().strip(),'CONFIGURE_LOCAL_CREDENTIAL'))
    local=ROOT/'.private/lyke-cloud/edge-secrets.env'
    if local.exists():
        for entry in local.read_text().splitlines():
            key,separator,value=entry.partition('=')
            if separator and re.search(r'KEY|TOKEN|SECRET|PASSWORD|HMAC',key):
                private_values.append((value.strip().strip('"\''),'CONFIGURE_LOCAL_CREDENTIAL'))
    copied = []
    redacted = []
    for path in sorted(set(candidates)):
        relative = path.relative_to(ROOT)
        destination = output/relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        if path.suffix.lower() in {'.png', '.jpg', '.jpeg', '.pdf'}:
            data=path.read_bytes()
            if any(value and value.encode() in data for value,_ in private_values):
                raise SystemExit(f'Local credential detected in binary document: {relative}')
            shutil.copy2(path, destination)
        else:
            text = path.read_text(encoding='utf-8-sig')
            original = text
            for value, replacement in private_values:
                if value:
                    text = text.replace(value, replacement)
            if TOKEN_PATTERN.search(text):
                raise SystemExit(f'Possible credential detected; publication refused: {relative}')
            destination.write_text(text, encoding='utf-8', newline='\n')
            if text != original:
                redacted.append(relative.as_posix())
        copied.append({'path': relative.as_posix(), 'bytes': destination.stat().st_size,
                       'sha256': hashlib.sha256(destination.read_bytes()).hexdigest()})
    (ROOT/'.private/lyke-cloud/repository-snapshot.json').write_text(json.dumps({
        'files': copied, 'redacted_files': redacted,
        'excluded': ['credentials', 'flash/NVS backups', 'private photographs', 'firmware binaries', 'build tools'],
    }, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'files': len(copied), 'redacted_files': len(redacted), 'output': str(output)}))


if __name__ == '__main__':
    main()
