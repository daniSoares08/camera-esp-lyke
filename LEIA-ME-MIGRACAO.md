# ESP32-S3-CAM GOOUUU — pacote de migração

## Uso neste computador (C:\Projects\CAMERA_ESP)

Use `Abrir camera.bat` na pasta principal. Os iniciadores agora usam o ambiente Python local `.venv`, e os scripts de foto/comparação usam USB OTG. O módulo informado neste PC é **DC5640-AF 78MM**. O perfil local foi ajustado para documentos com câmera apoiada: exposição automática livre e foco único. A pasta `outputs\diagnostico_pc` registra o estado da gravação, capturas e conclusões deste trabalho.

Arduino CLI 1.5.1 fica em `.tools\arduino-cli-stable`, com core ESP32 3.3.6. `Instalar ferramentas.ps1` reproduz a instalação. Use `firmware\Compilar firmware.ps1` para compilar o fonte (o script prepara uma pasta de sketch com o nome exigido pelo Arduino). `firmware\Gravar firmware.ps1 -Porta COM3` acompanha a mudança de porta USB para o bootloader e grava a aplicação preservando NVS. Os novos binários ficam em `firmware\release_pc`; `firmware\release` mantém os anteriores.

As seções seguintes são o histórico dos testes anteriores; os números de COM e o perfil de 12 ms ali mencionados não são o novo perfil para câmera apoiada.

Este diretório reúne o controlador USB, o firmware atual, os binários prontos, as fotos e medições produzidas durante os testes e este histórico. Foi preparado em 09/10/2026 para continuar o trabalho em outro computador Windows.

## Começar em outro computador

1. Instale Python 3 para Windows e, no terminal, rode `py -3 -m pip install pyserial pillow`.
2. Conecte a placa pela USB-C **OTG**. Para o firmware atual não é necessário conectar a USB TTL nem configurar Wi-Fi.
3. Abra `outputs\Abrir controle da camera.bat`, escolha a porta COM da Espressif e clique em **Conectar**. Os números COM podem mudar entre PCs. Se não aparecer nenhuma porta, confira o Gerenciador de Dispositivos e o cabo USB de dados.
4. Clique em **Tirar foto** ou pressione Espaço. As fotos novas são gravadas em `outputs\fotos`, com medições JSON ao lado. Use **Ver em 100%** e a barra de rolagem para examinar detalhes.

O painel requer Python 3, Pillow e pyserial. O perfil local está em `outputs\perfil_documento.json`; os ajustes também são guardados na memória não volátil (NVS) do ESP. Se trocar de módulo, desligue a placa antes de mexer no flat e selecione o módulo correspondente no painel.

## Equipamento e estado atual

- Placa: GOOUUU ESP32-S3-CAM V1.5, identificada nos testes como ESP32-S3-N16R8 (16 MB de flash e 8 MB de PSRAM OPI).
- Conexão usada agora: somente USB-C OTG no computador. O carregador externo foi retirado depois de observar no esquema publicado que as duas USB compartilham a rede USB_5V.
- Sensor detectado: OmniVision OV5640 (PID `0x5640`). O modo de maior resolução disponível neste firmware e confirmado nas imagens é **2560 × 1920 (4,9 MP)**; não 2592 × 1944.
- Dois módulos testados: `DC5640-AF 78MM` e `DCXYX-LZTKQJ-5M-357-V1 FF`. No painel, selecione o segundo como FF para suprimir comandos de foco automático. O código do flat, sozinho, não determina a distância ideal de foco.
- Firmware atual: USB CDC, sem inicialização de Wi-Fi. A fonte correspondente é `firmware\camera_usb`. O firmware contém comandos de exposição, ganho, balanço de branco, nitidez, contraste, saturação, compressão, resolução e foco quando há módulo compatível.
- Perfil recomendado para continuar: JPEG Q10, exposição automática limitada a 12 ms, ganho automático até 8×, balanço de branco automático, brilho/contraste/saturação 0 e nitidez 1. O perfil é um ponto de partida medido, não uma garantia de leitura óptica.

## O que fizemos até agora

1. Começamos identificando as duas USB-C: a TTL é útil para serial/programação por conversor USB-UART; a OTG oferece USB nativo do ESP32-S3 e é a conexão escolhida para controle e transferência de fotos. O desenvolvimento acabou ficando todo pelo OTG, sem depender de Wi-Fi.
2. Configuramos a captura USB e um painel desktop para controlar a câmera. O painel permite ajustar exposição, ganho/ISO equivalente, balanço de branco, brilho, contraste, saturação, nitidez, qualidade JPEG, resolução, espelhamento e controles de foco disponíveis. A interface recebeu rolagem vertical para mostrar todos os controles e uma visualização em 100% para examinar os pixels originais.
3. Capturamos a página inteira e comparamos compressões Q6, Q8, Q10 e Q15 sob a mesma exposição estimada (11,945 ms) e ganho (2,938×). Os tamanhos foram aproximadamente 524 KB, 396 KB, 321 KB e 224 KB. O envio USB medido foi 933, 709, 564 e 399 ms, respectivamente. Q10 foi escolhido pelo equilíbrio observado entre tamanho, tempo e aparência.
4. Ajustamos exposição e nitidez em várias condições. Exposição mais longa reduziu ruído, mas o desfoque observado permaneceu. Alterações de compressão, contraste e nitidez não recuperaram os caracteres pequenos. Converter JPEG para PNG também não recuperaria detalhes que o sensor não registrou.
5. A segunda câmera foi testada mais perto e com a alimentação externa removida. A folha ocupou mais pixels, mas a leitura completa ainda não foi validada. No teste somente OTG, a exposição estimada foi 11,945 ms, ganho 6,062×, envio USB 890 ms e total 1065 ms. Com exposição livre houve 50,339 ms; limitando a 25 ms, o sensor compensou com ganho 2,938×. A tentativa Q6 nesta cena falhou; Q10 foi restaurado.
6. O OCR local dos testes teve muitos erros e está guardado como saída bruta, sem correções. Não usamos esse OCR para afirmar que todos os textos, valores ou números do papel foram lidos corretamente.
7. Examinamos o esquema publicado da placa. Nele, os dois VBUS USB aparecem ligados à mesma linha USB_5V; U4/U5 alimentam a câmera por reguladores e os pinos FPC 23/24 aparecem sem conexão. O esquema é uma publicação GOOUUU, mas não foi confirmada sua correspondência exata com esta revisão V1.5 nem a pinagem de alimentação do atuador do módulo AF específico. Portanto, não concluímos que o firmware possa alimentar um motor por um pino ausente ou que a câmera esteja defeituosa.
8. Um protótipo antigo de servidor Wi-Fi foi usado em testes preliminares. Incluí a fonte em `firmware\legacy_wifi`, mas removi as credenciais e não copiei o binário antigo compilado com a senha embutida. O modelo `credentials.example.h` contém apenas marcadores. O firmware atual não inicializa Wi-Fi e não precisa de credenciais.

## Exposição não é o tempo de transferência

A exposição é o tempo em que cada linha do sensor recebe luz. Como o OV5640 usa obturador rolante, linhas diferentes são expostas em momentos diferentes. Ela afeta diretamente desfoque por movimento e quantidade de luz. O limite de 12 ms tenta reduzir movimento; em pouca luz, o controle aumenta o ganho, que pode aumentar o ruído.

O tempo USB é o envio dos bytes ao PC. A espera pelo quadro e o total também aparecem no painel. No teste Q10 da segunda câmera, por exemplo, a transferência levou 890 ms embora a exposição tenha sido cerca de 12 ms. Assim, o tempo total de chegada ao PC não significa que a câmera ficou capturando durante todo esse tempo. Para evitar tremor, o ponto crítico é a exposição; para receber a imagem, conta a transferência.

## Gravar o firmware existente

Os binários da compilação atual ficam em `firmware\release`. Para regravar por OTG em outro computador, instale esptool 5.1.0 (`py -3 -m pip install esptool==5.1.0`), coloque a placa em modo bootloader/Download (BOOT pressionado ao conectar ou enquanto toca RESET, conforme a placa) e confira a porta COM que aparece nesse modo. A porta pode mudar após entrar no bootloader.

No PowerShell, a partir de `E:\CAMERA_ESP`, substitua `COMx` pela porta do bootloader e rode:

```powershell
py -3 -m esptool --chip esp32s3 --port COMx --baud 460800 --before no-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 firmware\release\serial_camera_usb.ino.bootloader.bin 0x8000 firmware\release\serial_camera_usb.ino.partitions.bin 0xe000 firmware\release\boot_app0.bin 0x10000 firmware\release\serial_camera_usb.ino.bin
```

O arquivo `serial_camera_usb.ino.merged.bin` é a imagem consolidada de 16 MB, para uso somente se a ferramenta de gravação for configurada com endereço `0x0`. Os componentes separados e os endereços do comando acima são a opção mais explícita. Gravar firmware substitui o programa atual, mas não é preciso fazê-lo para usar a placa que já está configurada.

## Compilar a partir do fonte

Instale Arduino CLI e o pacote `esp32:esp32` versão 3.3.6. A placa foi compilada com o FQBN abaixo, incluindo PSRAM OPI, flash de 16 MB e CDC ligado no boot:

```powershell
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.6
arduino-cli compile --fqbn "esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,USBMode=default,CDCOnBoot=cdc,PartitionScheme=default" --build-path firmware\build firmware\camera_usb
```

O binário pronto foi produzido e gravado com sucesso nesta placa durante o trabalho. Depois de compilar novamente, use as saídas `.bootloader.bin`, `.partitions.bin`, `boot_app0.bin` e `.bin` e os endereços acima. A compilação requer a toolchain ESP32; copiar apenas o fonte não instala essa toolchain.

## Pastas

- `outputs\`: painel, protocolo USB, iniciadores `.bat`, perfil e todas as capturas/relatórios destinados ao usuário.
- `firmware\camera_usb\`: fonte Arduino atual e firmware de inicialização AF do OV5640.
- `firmware\release\`: binário de aplicação, bootloader, partições, `boot_app0`, imagem consolidada, argumentos e configuração usados na compilação.
- `firmware\legacy_wifi\`: fonte sanitizada do protótipo de rede e modelo sem segredos; exige credenciais locais se for compilado.
- `dados\laboratorio\`: scripts, imagens de análise, esquemas e logs técnicos que complementam os relatórios.

O diretório `outputs\camera_ff` registra as capturas da segunda câmera e `outputs\calibracao` guarda comparações da primeira rodada. Os arquivos `*_ocr.txt` são resultados automáticos com erros; confira as fotos originais antes de qualquer conclusão.

## Referências da placa

O esquema e as imagens publicados foram incluídos em `outputs\GOOUUU_esquema_publicado.pdf` e `dados\laboratorio\GOOUUU_schematic.pdf`. Repositório do fabricante/comunidade: <https://github.com/zhuhai-esp/ESP32-S3-Goouuu-Cam>. Use o esquema como referência da versão publicada, não como prova definitiva de que todas as revisões de placa sejam idênticas.



