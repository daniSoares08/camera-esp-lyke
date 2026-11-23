param([Parameter(Mandatory=$true)][ValidateSet('S3','Devkit')][string]$Alvo,[Parameter(Mandatory=$true)][ValidatePattern('^COM\d+$')][string]$Porta)
$ErrorActionPreference='Stop'
$projectDir=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$python=Join-Path $projectDir '.venv\Scripts\python.exe'
$name=if($Alvo -eq 'S3'){'s3_calculadora'}else{'devkit_display'}
$chip=if($Alvo -eq 'S3'){'esp32s3'}else{'esp32'}
$size=if($Alvo -eq 'S3'){'16MB'}else{'4MB'}
$bin=Join-Path $PSScriptRoot ('bin\'+$name)
$manifest=Get-Content -LiteralPath (Join-Path $bin 'manifest.json') -Raw | ConvertFrom-Json
$names=@("$name.ino.bootloader.bin","$name.ino.partitions.bin",'boot_app0.bin',"$name.ino.bin")
foreach($fileName in $names){$path=Join-Path $bin $fileName;$expected=$manifest.files.PSObject.Properties[$fileName].Value.sha256;if(-not $expected -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected){throw ('Binario alterado ou ausente: '+$fileName)}}
$bootOffset=if($Alvo -eq 'S3'){'0x0'}else{'0x1000'}
Write-Host "Gravando $Alvo em $Porta. Use a USB/UART correspondente e feche outros monitores."
& $python -m esptool --chip $chip --port $Porta --baud 460800 --before default-reset --after hard-reset write-flash --flash-mode dio --flash-freq 80m --flash-size $size `
  $bootOffset (Join-Path $bin $names[0]) `
  0x8000 (Join-Path $bin $names[1]) `
  0xe000 (Join-Path $bin $names[2]) `
  0x10000 (Join-Path $bin $names[3])
if($LASTEXITCODE -ne 0){throw 'Falha na gravacao. Confira placa, porta e modo BOOT.'}
