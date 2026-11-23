$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cli = Join-Path $projectDir '.tools\arduino-cli-stable\arduino-cli.exe'
$buildDir = Join-Path $projectDir '.build\mapear_teclado'
$binDir = Join-Path $PSScriptRoot 'bin'
New-Item -ItemType Directory -Force $buildDir, $binDir | Out-Null
$fqbn = 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,USBMode=hwcdc,CDCOnBoot=default,PartitionScheme=default'
& $cli compile --fqbn $fqbn --build-path $buildDir --output-dir $binDir $PSScriptRoot
if ($LASTEXITCODE -ne 0) { throw 'Compilacao falhou.' }
$options = Get-Content -LiteralPath (Join-Path $buildDir 'build.options.json') -Raw | ConvertFrom-Json
$coreDir = ($options.hardwareFolders -split ',')[0]
Copy-Item -LiteralPath (Join-Path $coreDir 'tools\partitions\boot_app0.bin') -Destination (Join-Path $binDir 'boot_app0.bin')
