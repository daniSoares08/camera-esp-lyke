param([Parameter(Mandatory = $true)][ValidatePattern('^COM\d+$')][string]$Porta,[ValidateSet('S3','ESP32CAM')][string]$Placa='S3')
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $PSScriptRoot
$pythonExe = Join-Path $projectDir '.venv\Scripts\python.exe'
if($Placa -eq 'ESP32CAM'){
    $releaseDir = Join-Path $PSScriptRoot 'release_esp32cam'
    $manifest = Get-Content -LiteralPath (Join-Path $releaseDir 'manifest.json') -Raw | ConvertFrom-Json
    foreach($name in @('serial_camera_usb.ino.bootloader.bin','serial_camera_usb.ino.partitions.bin','boot_app0.bin','serial_camera_usb.ino.bin')){
        $file = Join-Path $releaseDir $name
        if((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifest.$name.sha256){throw ('Hash incorreto: '+$name)}
    }
    & $pythonExe -m esptool --chip esp32 --port $Porta --baud 460800 --before default-reset --after hard-reset write-flash --flash-mode dio --flash-freq 80m --flash-size 4MB 0x1000 (Join-Path $releaseDir 'serial_camera_usb.ino.bootloader.bin') 0x8000 (Join-Path $releaseDir 'serial_camera_usb.ino.partitions.bin') 0xe000 (Join-Path $releaseDir 'boot_app0.bin') 0x10000 (Join-Path $releaseDir 'serial_camera_usb.ino.bin')
    if($LASTEXITCODE -ne 0){throw 'Gravacao ESP32-CAM falhou.'}
    return
}
$applicationBin = Join-Path $PSScriptRoot 'release_pc\serial_camera_usb.ino.bin'
if (-not (Test-Path -LiteralPath $applicationBin)) { throw 'Execute Compilar firmware.ps1 primeiro.' }
# Mesmo mapa de partições do firmware anterior. Preserva bootloader e NVS.
& $pythonExe (Join-Path $PSScriptRoot 'gravar_usb.py') --port $Porta
if ($LASTEXITCODE -ne 0) { throw 'Gravação falhou. Feche o painel e verifique a porta OTG/BOOT.' }
