# ESP32-CAM clássica: câmera, fila de fotos e atualização Wi-Fi

Alvo exclusivo: **ESP32-D0WD-V3 / perfil AI Thinker, flash 4 MiB,
PSRAM 4 MiB**. Não usar estes arquivos na GOOUUU ESP32-S3 nem no DevKit
da calculadora antiga. O fonte e os binários de câmera r6 originais ficam
preservados em `firmware/camera_usb` e `firmware/release_esp32cam`.

## O que está implementado

- Câmera JPEG com os controles UART/autofoco do r6, usando um framebuffer.
  A compilação copia o fonte original para uma área de trabalho e renomeia
  as funções de entrada; não regrava o fonte de bancada original.
- Falta de câmera/PCF não impede boot, diagnóstico ou atualização.
- Fila de até 16 entradas, limitada principalmente por bytes: 1,5 MiB de
  JPEGs na PSRAM e 600 KiB na flash, com reserva adicional no filesystem.
  JPEGs persistentes são recuperados no boot; arquivos parciais são descartados.
- Galeria/download no navegador e captura local explícita.
- OTA em dois slots de 1,5 MiB. Recebimento em blocos, validação de tamanho,
  chip ESP32 clássico, identificação do projeto e integridade nativa da imagem.
  SHA-256 de transporte também é conferido quando fornecido pelo cliente.
  Arquivo incorreto não altera o slot selecionado para boot.
- Ao entrar no OTA, desliga a interface da câmera e apaga a fila de fotos,
  incluindo os JPEGs da flash. Não apaga NVS/configurações.
- PCF8575 no I2C0 (GPIO13/14), endereço 0x20. O SCCB da câmera usa I2C1
  neste SDK, confirmado na bancada. Imagem única de 16 bits
  preserva os sinais de teclado e LCD durante as operações.
- Varredura/debounce da matriz 4x5; emite coordenadas lógicas pela UART.
  Teclas simultâneas ambíguas são ignoradas. A ordem física ainda precisa
  ser medida; não há associação presumida das 20 posições a teclas.
- Driver pequeno ST7565R, framebuffer de 1 KiB e fonte 5x7, habilitado
  explicitamente. GPIO15/2 e CS/RESET/RS via P06/P07/P11 do PCF.
- Núcleo matemático existente reaproveitado para cálculos pela UART.

Esta é a base de integração e teste OTA. **Não inclui ainda consulta HTTPS/API,
provisionamento de redes/chave, menu completo da calculadora ou associação do
teclado físico às ações.** Esses recursos exigem outra medição de tamanho.
Nenhuma chamada paga à API é feita por esta versão.

LCD/teclado/câmera/bateria desconectados não constituem validação física de
seus drivers. Contraste/orientação do LCD precisam de bancada. ADC33 fica
desabilitado até comando explícito, após isolar o LED vermelho e montar o
divisor conforme o esquema. Após ligar/acordar, aguardar pelo menos 200 ms
antes de consultar o ADC.

## Compilar

Na raiz do projeto:

```powershell
.venv\Scripts\python.exe firmware/esp32cam_integrada/build.py --revision cam-ota-r1-usb --optimization baseline
.venv\Scripts\python.exe firmware/esp32cam_integrada/build.py --revision cam-ota-r2-wifi --optimization lto
```

Core ESP32 3.3.6, perfil `esp32:esp32:esp32cam`, otimização `-Os` em ambos.
A opção `lto` acrescenta otimização entre módulos para o sketch e código C++
compilado de fonte. O código C permanece em `-Os`: LTO também no C do core
provocou erro de relocação Xtensa no wrapper de panic/backtrace, sem gerar
binário utilizável. Não se ignorou esse erro do linker. As bibliotecas IDF
pré-compiladas continuam as fornecidas pelo core.
Não usa Bluetooth, grandes fontes U8g2 nem ArduinoJson para esta interface.
Nenhuma credencial privada/API é incorporada ao binário.

O core Arduino vem com identificação genérica em `esp_app_desc`. O script
ajusta **a seção do ELF** com o nome do projeto e a revisão e depois regenera
a imagem completa com esptool, incluindo os hashes/checksums. Não altera
diretamente um `.bin` pronto. Cada release tem manifesto de tamanho/SHA-256.

## Primeira instalação e atualizações

A primeira instalação exige USB para estabelecer as duas partições OTA.
O script inicial reidentifica o FTDI e exige backup dos primeiros 64 KiB
(bootloader/tabela/NVS), confere hashes e verifica o aplicativo original r6
antes de aceitar a migração. A NVS em 0x9000 é preservada.

```powershell
.venv\Scripts\python.exe firmware/esp32cam_integrada/flash_initial.py --port COM17 --revision cam-ota-r1-usb
```

COM17 é histórica: reidentificar. Depois da migração, usar somente
**esp32cam_integrada.ino.bin** no OTA, nunca merged/bootloader/partitions.
A tabela mantém a área de arquivos em 0x310000, tamanho 896 KiB.
O sistema de arquivos desta aplicação é LittleFS; a formatação inicial é
explícita pelo comando `FORMAT`, não automática em caso de falha de montagem.

UART: **460800 baud**, DTR/RTS desativados ao abrir em conversor FTDI.
Comandos terminam com quebra de linha:

| Comando | Ação |
|---|---|
| `STATUS` | JSON de versão, slots, memória, periféricos e fotos |
| `WIFI` | Abre AP/galeria local |
| `OTA` | Limpa fotos e abre AP em modo atualização |
| `OFF` | Encerra Wi-Fi, se não houver atualização em andamento |
| `SAVE` | Captura e guarda um JPEG na fila |
| `CAMERA` | Tenta inicializar/reinicializar câmera |
| `CLEAR` | Apaga fotos da fila |
| `FORMAT` | Formata explicitamente a área LittleFS de fotos |
| `LCD ON` | Inicializa o LCD se o PCF responder |
| `ADC ON` / `ADC` | Habilita a leitura / consulta tensão da bateria |
| `CALC 2+3*4` | Testa núcleo matemático, retorna 14 |
| `REBOOT` | Reinicia sem gravar firmware |
| `BENCH` | Habilita importação de JPEGs para teste sem câmera |

Com câmera ativa, os comandos de bancada `I/P/V/F/S/C/Q` continuam disponíveis.
A revisão `cam-ota-r4-clean` remove o teste temporário de LED33 da r3, após o
usuário informar que dessoldou o LED vermelho. Não há comandos de piscadas;
GPIO33 inicia como entrada sem pull e não é acionado como saída pelo firmware.
Leitura de bateria continua desabilitada até `ADC ON`; pad/fio/divisor ainda
precisam de confirmação física.
`SAVE` retorna o sensor ao power-down e libera driver/XCLK depois de copiar
o JPEG. Isso não significa corte físico da alimentação.

AP: `EspCam-…`, senha local solicitada pelo usuário **CHANGE_ME_ESP_AP**,
endereço **http://192.168.8.1**. Só fica ativo após comando, sem expiração
durante o OTA. O recebimento usa token de sessão e fica limitado à interface
local do AP. O token não é salvo nos relatórios. Reiniciar volta com Wi-Fi
desligado; UART permite reabrir o modo sem tela/teclado.

O OTA preserva a imagem em execução durante o recebimento. **Rollback
automático por falha funcional da nova aplicação não foi habilitado** no
bootloader pré-compilado utilizado. O teste verifica explicitamente boot,
versão, slot e digest; isso não substitui implementar rollback se requerido.

## Teste físico de OTA

```powershell
.venv\Scripts\python.exe firmware/esp32cam_integrada/verify_ota.py --port COM17 --interface-index 17 --binary firmware/esp32cam_integrada/bin/cam-ota-r2-wifi/esp32cam_integrada.ino.bin --report outputs/esp32cam_ota/verificacao.json --restore-radio-off
```

Reidentificar porta/interface antes de executar. O teste controla o rádio
Wi-Fi pela API nativa Windows e cria/remove um perfil temporário. Usa conexão
HTTP vinculada à interface Wi-Fi, sem proxy; a Internet do PC continua pelo
Ethernet. `--restore-radio-off` corresponde ao estado observado nesta bancada;
omitir quando o rádio já estiver ligado e deva continuar assim.

O envio do `.bin` é por Wi-Fi. USB é usado somente para comandos e telemetria.
Importa quatro cópias de um JPEG real de 498.378 bytes: três na PSRAM e uma
na flash; verifica os downloads, o limite de memória, a persistência após
reboot e a limpeza no OTA. Verifica rejeição de arquivo sem autorização,
nome/chip/projeto incorretos, tamanho excedido, truncamento e corrupção.
Só conclui sucesso após a versão nova responder no outro slot e por Wi-Fi.

O teste não corta energia durante a gravação e não mede corrente de bateria.

## Fontes técnicas

- `outputs/ligacoes/ESP-CAM-esquema.html` e guia/JSON correspondentes.
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html
- https://ae-pic-a1.aliexpress-media.com/kf/S3dd8f051c7fc44a2873b1aee32568c34c.pdf
- https://learn.microsoft.com/en-us/windows/win32/api/wlanapi/ns-wlanapi-wlan_radio_state
