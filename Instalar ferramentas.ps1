$ErrorActionPreference = 'Stop'
$projectDir = $PSScriptRoot
$pythonExe = Join-Path $projectDir '.venv\Scripts\python.exe'
if (-not (Test-Path -LiteralPath $pythonExe)) {
    & py -3.11 -m venv (Join-Path $projectDir '.venv')
    if ($LASTEXITCODE -ne 0) { throw 'Instale Python 3.11 com tkinter e tente novamente.' }
}
& $pythonExe -m pip install -r (Join-Path $projectDir 'requirements.txt')
if ($LASTEXITCODE -ne 0) { throw 'Falha na instalação das dependências Python.' }
$toolDir = Join-Path $projectDir '.tools\arduino-cli-stable'
$arduinoExe = Join-Path $toolDir 'arduino-cli.exe'
if (-not (Test-Path -LiteralPath $arduinoExe)) {
    New-Item -ItemType Directory -Force $toolDir | Out-Null
    $zipFile = Join-Path $projectDir '.tools\arduino-cli-stable.zip'
    Invoke-WebRequest 'https://github.com/arduino/arduino-cli/releases/download/v1.5.1/arduino-cli_1.5.1_Windows_64bit.zip' -OutFile $zipFile
    Expand-Archive -LiteralPath $zipFile -DestinationPath $toolDir -Force
}
& $arduinoExe core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
if ($LASTEXITCODE -ne 0) { throw 'Falha ao atualizar os índices Arduino.' }
& $arduinoExe core install esp32:esp32@3.3.6 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
if ($LASTEXITCODE -ne 0) { throw 'Falha ao instalar o pacote ESP32.' }
& $pythonExe -c 'import serial, PIL, tkinter, esptool; print("Python, USB serial, imagens e gravacao prontos.")'
if ($LASTEXITCODE -ne 0) { throw 'Falha na verificação das bibliotecas.' }
Write-Host 'Abra Abrir camera.bat para usar a câmera.'
