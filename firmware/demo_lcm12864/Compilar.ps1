$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cli = Join-Path $projectDir '.tools\arduino-cli-stable\arduino-cli.exe'
$buildDir = Join-Path $projectDir '.build\demo_lcm12864'
$binDir = Join-Path $PSScriptRoot 'bin'
New-Item -ItemType Directory -Force $buildDir, $binDir | Out-Null
$fqbn = 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,USBMode=default,CDCOnBoot=cdc,PartitionScheme=default'
& $cli compile --fqbn $fqbn --build-path $buildDir --output-dir $binDir $PSScriptRoot
if ($LASTEXITCODE -ne 0) { throw 'Compilacao falhou.' }
# Arduino CLI nao exporta o seletor de particao OTA com --output-dir.
$buildOptions = Get-Content -LiteralPath (Join-Path $buildDir 'build.options.json') -Raw | ConvertFrom-Json
$coreDir = ($buildOptions.hardwareFolders -split ',')[0]
$bootAppFile = Join-Path $coreDir 'tools\partitions\boot_app0.bin'
if (-not (Test-Path -LiteralPath $bootAppFile)) { throw 'boot_app0.bin ausente no core ESP32.' }
Copy-Item -LiteralPath $bootAppFile -Destination (Join-Path $binDir 'boot_app0.bin')
$manifest = @{}
Get-ChildItem -LiteralPath $binDir -Filter '*.bin' | ForEach-Object {
    $manifest[$_.Name] = @{ bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
}
$manifest | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $binDir 'manifest.json') -Encoding utf8
