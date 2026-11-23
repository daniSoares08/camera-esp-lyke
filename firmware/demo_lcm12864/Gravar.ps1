param([Parameter(Mandatory = $true)][ValidatePattern('^COM\d+$')][string]$Porta)
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$pythonExe = Join-Path $projectDir '.venv\Scripts\python.exe'
$releaseDir = Join-Path $PSScriptRoot 'bin'
if (-not (Test-Path -LiteralPath $pythonExe)) { throw 'Python local ausente. Veja GUIA-LIGACAO.md.' }
$names = @('demo_lcm12864.ino.bootloader.bin', 'demo_lcm12864.ino.partitions.bin', 'boot_app0.bin', 'demo_lcm12864.ino.bin')
$manifestPath = Join-Path $releaseDir 'manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath)) { throw 'Execute Compilar.ps1 primeiro ou use o pacote completo de binarios.' }
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
foreach ($name in $names) {
    $file = Join-Path $releaseDir $name
    if (-not (Test-Path -LiteralPath $file)) { throw "Binario ausente: $name" }
    $expected = $manifest.PSObject.Properties[$name].Value.sha256
    $actual = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($expected -ne $actual) { throw "Hash diferente: $name. Recompile antes de gravar." }
}
Write-Host 'A placa deve estar em modo Download (BOOT + RESET). Isto substitui o programa atual.'
& $pythonExe -m esptool --chip esp32s3 --port $Porta --baud 460800 --before no-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 80m --flash-size 16MB `
    0x0 (Join-Path $releaseDir $names[0]) `
    0x8000 (Join-Path $releaseDir $names[1]) `
    0xe000 (Join-Path $releaseDir $names[2]) `
    0x10000 (Join-Path $releaseDir $names[3])
if ($LASTEXITCODE -ne 0) { throw 'Gravacao falhou. Confira a porta do bootloader, cabo e se o monitor serial esta fechado.' }
Write-Host 'Gravado. Se a demo nao iniciar, solte BOOT e pressione RESET.'

