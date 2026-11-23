param([ValidateSet('Todos','S3','Devkit')][string]$Alvo='Todos')
$ErrorActionPreference='Stop'
$projectDir=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cli=Join-Path $projectDir '.tools\arduino-cli-stable\arduino-cli.exe'
$targets=@()
if($Alvo -in @('Todos','S3')){$targets+=@{name='s3_calculadora';fqbn='esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,USBMode=hwcdc,CDCOnBoot=default,PartitionScheme=huge_app'}}
if($Alvo -in @('Todos','Devkit')){$targets+=@{name='devkit_display';fqbn='esp32:esp32:esp32:FlashSize=4M,PartitionScheme=huge_app'}}
foreach($target in $targets){
  $stage=Join-Path $projectDir ('.build\calculadora\sketches\'+$target.name)
  $build=Join-Path $projectDir ('.build\calculadora\build_'+$target.name)
  $bin=Join-Path $PSScriptRoot ('bin\'+$target.name)
  New-Item -ItemType Directory -Force $stage,$build,$bin | Out-Null
  Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot $target.name) -File | Where-Object {$_.Extension -in @('.ino','.h')} | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $stage}
  Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'common') -Filter '*.h' | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $stage}
  Copy-Item -LiteralPath (Join-Path $PSScriptRoot ($target.name+'\partitions.csv')) -Destination $stage
  if($target.name -eq 's3_calculadora'){
    $secrets=Join-Path $PSScriptRoot 'secrets.local.h'
    if(-not (Test-Path -LiteralPath $secrets)){throw 'Crie secrets.local.h a partir de secrets.example.h.'}
    Copy-Item -LiteralPath $secrets -Destination $stage
  }
  & $cli compile --fqbn $target.fqbn --build-path $build --output-dir $bin $stage
  if($LASTEXITCODE -ne 0){throw ('Falha ao compilar '+$target.name)}
  $otaCapacity=if($target.name -eq 's3_calculadora'){0x300000}else{0x180000}
  if((Get-Item -LiteralPath (Join-Path $bin ($target.name+'.ino.bin'))).Length -gt $otaCapacity){throw ('Firmware excede o espaco OTA: '+$target.name)}
  $options=Get-Content -LiteralPath (Join-Path $build 'build.options.json') -Raw | ConvertFrom-Json
  $core=($options.hardwareFolders -split ',')[0]
  Copy-Item -LiteralPath (Join-Path $core 'tools\partitions\boot_app0.bin') -Destination $bin
  $manifest=@{fqbn=$target.fqbn;target=$target.name;ota_capacity=$otaCapacity;files=@{}}
  Get-ChildItem -LiteralPath $bin -Filter '*.bin' | ForEach-Object {$manifest.files[$_.Name]=@{bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}}
  $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $bin 'manifest.json') -Encoding utf8
}
