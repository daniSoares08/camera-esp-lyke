$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$arduinoExe = Join-Path $projectDir '.tools\arduino-cli-stable\arduino-cli.exe'
if (-not (Test-Path -LiteralPath $arduinoExe)) { throw 'Arduino CLI ausente. Veja GUIA-LIGACAO.md.' }
$buildDir = Join-Path $projectDir '.build\demo_gm12864'
$releaseDir = Join-Path $PSScriptRoot 'bin'
New-Item -ItemType Directory -Force $buildDir, $releaseDir | Out-Null
$fqbn = 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,USBMode=default,CDCOnBoot=cdc,PartitionScheme=default'
& $arduinoExe compile --fqbn $fqbn --build-path $buildDir --output-dir $releaseDir $PSScriptRoot
if ($LASTEXITCODE -ne 0) { throw 'Compilacao falhou. Nenhum firmware foi gravado.' }
# O Arduino CLI nao exporta boot_app0.bin com --output-dir.
$buildOptions = Get-Content -LiteralPath (Join-Path $buildDir 'build.options.json') -Raw | ConvertFrom-Json
$coreDir = ($buildOptions.hardwareFolders -split ',')[0]
$bootAppFile = Join-Path $coreDir 'tools\partitions\boot_app0.bin'
if (-not (Test-Path -LiteralPath $bootAppFile)) { throw 'boot_app0.bin ausente no core ESP32 utilizado.' }
Copy-Item -LiteralPath $bootAppFile -Destination (Join-Path $releaseDir 'boot_app0.bin')
$manifest = @{}
Get-ChildItem -LiteralPath $releaseDir -Filter '*.bin' | ForEach-Object {
    $manifest[$_.Name] = @{
        bytes = $_.Length
        sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$manifest | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $releaseDir 'manifest.json') -Encoding utf8
Write-Host "Compilado. Binarios: $releaseDir"
