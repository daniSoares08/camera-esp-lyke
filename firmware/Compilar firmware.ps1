param([ValidateSet('S3','ESP32CAM')][string]$Placa='S3')
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $PSScriptRoot
$arduinoExe = Join-Path $projectDir '.tools\arduino-cli-stable\arduino-cli.exe'
$sketchDir = Join-Path $projectDir '.build\serial_camera_usb'
$buildDir = Join-Path $projectDir '.build\firmware'
$releaseDir = Join-Path $PSScriptRoot 'release_pc'
if($Placa -eq 'ESP32CAM'){
    $sketchDir = Join-Path $projectDir '.build\esp32cam\serial_camera_usb'
    $buildDir = Join-Path $projectDir '.build\esp32cam\build'
    $releaseDir = Join-Path $PSScriptRoot 'release_esp32cam'
}
New-Item -ItemType Directory -Force $sketchDir, $buildDir, $releaseDir | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'camera_usb\serial_camera_usb.ino') -Destination $sketchDir
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'camera_usb\ov5640_af_firmware.h') -Destination $sketchDir
$fqbn = 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,USBMode=default,CDCOnBoot=cdc,PartitionScheme=default'
if($Placa -eq 'ESP32CAM'){$fqbn = 'esp32:esp32:esp32cam'}
& $arduinoExe compile --fqbn $fqbn --build-path $buildDir --output-dir $releaseDir $sketchDir
if ($LASTEXITCODE -ne 0) { throw 'A compilação falhou; firmware não foi gravado.' }
$options=Get-Content -LiteralPath (Join-Path $buildDir 'build.options.json') -Raw | ConvertFrom-Json
$core=($options.hardwareFolders -split ',')[0]
Copy-Item -LiteralPath (Join-Path $core 'tools\partitions\boot_app0.bin') -Destination $releaseDir
Get-FileHash -Algorithm SHA256 (Join-Path $releaseDir '*.bin') | Format-Table -AutoSize
$manifest = @{}
Get-ChildItem -LiteralPath $releaseDir -Filter '*.bin' | ForEach-Object {
    $manifest[$_.Name] = @{ bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
}
$manifest | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $releaseDir 'manifest.json') -Encoding utf8
Write-Host "Binários: $releaseDir"
