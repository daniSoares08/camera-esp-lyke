param([Parameter(Mandatory = $true)][ValidatePattern('^COM\d+$')][string]$Porta)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$python = Join-Path $root '.venv\Scripts\python.exe'
$bin = Join-Path $PSScriptRoot 'bin'
Write-Host 'Use USB/UART, com o cabo OTG desconectado. Substituindo o firmware atual.'
& $python -m esptool --chip esp32s3 --port $Porta --baud 460800 --before default-reset --after hard-reset write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB `
  0x0 (Join-Path $bin 'mapear_teclado.ino.bootloader.bin') `
  0x8000 (Join-Path $bin 'mapear_teclado.ino.partitions.bin') `
  0xe000 (Join-Path $bin 'boot_app0.bin') `
  0x10000 (Join-Path $bin 'mapear_teclado.ino.bin')
if ($LASTEXITCODE -ne 0) { throw 'Falha na gravacao. Confira a porta USB/UART.' }
