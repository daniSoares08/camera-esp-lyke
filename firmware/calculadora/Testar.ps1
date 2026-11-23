$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$build=Join-Path $root '.build\calculadora'
$out=Join-Path $root 'outputs\calculadora\previas'
New-Item -ItemType Directory -Force $build,$out | Out-Null
$json=Join-Path $env:USERPROFILE 'Documents\Arduino\libraries\ArduinoJson\src'
& g++ -std=c++17 -O2 -Wall -Wextra -I $json (Join-Path $PSScriptRoot 'tests\test_core.cpp') -o (Join-Path $build 'test_core.exe')
if($LASTEXITCODE -ne 0){throw 'Falha na compilacao dos testes.'}
& (Join-Path $build 'test_core.exe') $out
if($LASTEXITCODE -ne 0){throw 'Falha nos testes.'}
& (Join-Path $root '.venv\Scripts\python.exe') -m unittest discover -s (Join-Path $PSScriptRoot 'tests') -p 'test_*.py' -v
if($LASTEXITCODE -ne 0){throw 'Falha nos testes Python.'}
& node (Join-Path $PSScriptRoot 'tests\test_photo_viewer.cjs')
if($LASTEXITCODE -ne 0){throw 'Falha nos testes da galeria.'}
