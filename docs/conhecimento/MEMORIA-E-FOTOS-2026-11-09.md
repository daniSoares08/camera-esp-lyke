# Memória e fila de fotos — ESP32-CAM clássica

**Atualização posterior nesta mesma data:** o usuário autorizou implementação
e gravação. A versão `cam-ota-r2-wifi` foi instalada por OTA e os testes de
fila/atualização passaram. Ver `ESTADO-E-RETOMADA.md` e
`../../firmware/esp32cam_integrada/LEIA-ME.md`; as seções abaixo preservam as
estimativas e o estado anteriores à implementação. Agora há teste real de
quatro JPEGs de 498.378 bytes (3 PSRAM + 1 flash), importados sem sensor.

Inspeção em 09/11/2026 (America/Sao_Paulo). Usuário informa placa sem câmera
e sem periféricos, conectada apenas ao computador. Nenhuma gravação de flash,
mudança de ligação ou integração de firmware foi realizada. A identificação
com esptool usa um stub temporário na RAM e reinicia a placa ao terminar.

## Verificado nesta inspeção

- Uma porta serial: COM17, FTDI VID0403/PID6001, serial A5069RR4A.
- `esptool 5.1.0 flash-id`: ESP32-D0WD-V3, revisão 3.1, dois núcleos,
  capacidade de 240 MHz, cristal de 40 MHz; Wi-Fi e Bluetooth.
- Flash detectada: 4 MiB (4.194.304 bytes), fabricante a1, dispositivo 4016.
- `verify-flash 0x10000 firmware/release_esp32cam/serial_camera_usb.ino.bin`:
  digest correspondente para todos os 405.168 bytes do aplicativo r6.
  Isso verifica o aplicativo; não foi uma leitura/verificação da flash inteira.
- Consulta `I` a 460800 baud: `CAMERA_ERROR 106`. Sem câmera, o firmware
  bloqueia os comandos antes de imprimir a telemetria de memória. Não foi
  medida a RAM livre atual. Pelo fluxo do fonte verificado, a inicialização
  passou pelo teste `psramFound()` antes de falhar na câmera.
- SRAM interna nominal do chip: 520 KB, compartilhada entre instruções,
  dados e sistema; esse total não corresponde ao heap livre.
- Perfil de compilação local: `esp32:esp32:esp32cam`, CPU padrão 240 MHz,
  PSRAM habilitada. Rádio do ESP32: Wi-Fi 2,4 GHz 802.11 b/g/n e BT 4.2/BLE.
- Firmware confirmado: `uart-camera-esp32cam-20261108-r6`; JPEG, dois
  framebuffers na PSRAM, QSXGA padrão. Sem Wi-Fi, tela, teclado ou fila de
  fotos implementados nesse alvo.

## Medições anteriores usadas, sem apresentá-las como medições de hoje

`outputs/teste_esp32cam_2026-11-08/verificacao_af_r6.json` registra:

- PSRAM total: 4.194.304 bytes = 4 MiB.
- PSRAM livre com câmera inicializada: 2.222.460 bytes = 2,1195 MiB.
- Diferença entre total e livre: 1.971.844 bytes = 1,8805 MiB. É o uso
  total observado nessa região, predominantemente associado à câmera;
  não é uma reserva medida de Wi-Fi, pois esse firmware não o inicializa.
- JPEG Q10, 2560 × 1920: 208.029 bytes (203,15 KiB), confirmado pelo r6.

Na mesma pasta, `teste_af.jpg` tem 498.378 bytes (486,70 KiB), também
2560 × 1920, de teste anterior dessa bancada. Não foram usados arquivos
da calculadora S3 para atribuir capacidade à placa clássica.

## Estimativa para o firmware futuro

Orçamento inicial proposto: **1,5 MiB (1.572.864 bytes) para a fila de JPEGs**,
mantendo aproximadamente 649.596 bytes da PSRAM livre histórica fora da fila.
Essa margem é uma escolha de projeto, não um ensaio de Wi-Fi/TLS integrado.
Também é necessário preservar heap interno e verificar o maior bloco livre;
PSRAM disponível não substitui toda a memória interna exigida pelo sistema.

| Tamanho de cada JPEG | Quantidade no orçamento de 1,5 MiB |
|---|---:|
| 200 KiB | 7 |
| 300 KiB | 5 |
| 500 KiB | 3 |
| 750 KiB | 2 |
| 1 MiB | 1 |

Para os dois arquivos medidos acima, caberiam respectivamente 7 e 3 cópias
nesse orçamento, desconsiderando os pequenos metadados. JPEG varia com cena,
iluminação e compressão: isso não garante uma quantidade fixa para toda foto.
Um alvo inicial razoável é **três fotos de alta resolução**, condicionado ao
tamanho total e à memória disponível. Antes de aceitar cada foto, conferir
espaço e tratar falha de alocação; não prometer três JPEGs arbitrariamente grandes.

A tela monocromática 128 × 64 do esquema precisa de 1.024 bytes para um
framebuffer completo de 1 bit por pixel. Teclado 4 × 5 e estado do PCF8575
consomem pouco; fontes, histórico, tarefas e bibliotecas acrescentam memória.
O código principal ocupa flash, enquanto variáveis, pilhas e buffers usam RAM.
Os 4 MiB de flash não são mais 4 MiB de RAM para fotos.

Na implementação, avaliar um único framebuffer para capturas sob demanda,
copiar somente o JPEG útil para PSRAM e liberar os recursos da câmera antes
do envio quando possível. Transmitir em fluxo para evitar cópias completas,
inclusive uma segunda representação integral em Base64. Testar com câmera,
tela, teclado e Wi-Fi/HTTPS ativos para definir o limite final.

Fotos mantidas apenas em RAM/PSRAM desaparecem ao reiniciar ou desligar.
O esquema atual não inclui microSD; persistência em flash exige planejamento
de partições e implementação próprios.

## Complemento: uso da flash para fotos

Na continuação da consulta em 09/11, a porta COM17 foi reidentificada e
`verify-flash 0x8000 firmware/release_esp32cam/serial_camera_usb.ino.partitions.bin`
confirmou correspondência dos 3.072 bytes da tabela de partições instalada,
sem gravação. A tabela reserva 3.145.728 bytes (3 MiB) para app0 e 917.504
bytes (896 KiB) para SPIFFS, além de NVS, dados OTA e coredump. Não há um
segundo slot de aplicativo nessa tabela. O tamanho do aplicativo r6 é
405.168 bytes; o espaço não ocupado dentro do slot app0 não é automaticamente
espaço de arquivos.

É possível implementar uma fila persistente de JPEGs na flash (por exemplo,
LittleFS), mantendo somente a captura e os blocos de envio em RAM. A partição
atual permite aproximadamente 1–3 fotos de 200–500 KiB com margem de sistema
de arquivos; depende do tamanho e ocupação real. Não houve montagem, leitura
de arquivos ou medição de espaço livre do sistema de arquivos nesta consulta.

Uma tabela futura poderia reservar cerca de 2 MiB para um aplicativo e cerca
de 1,8 MiB para arquivos, deixando espaço para boot/configuração. É apenas
uma proposta: o firmware integrado precisa ser compilado para validar seu
tamanho. OTA com dois aplicativos exige outra divisão e reduz o espaço
disponível para fotos. Nada foi reparticionado ou implementado.

Flash mantém arquivos concluídos ao desligar. Preferir salvar capturas
selecionadas, em vez de gravar continuamente a prévia, por latência e desgaste.
Uma fila híbrida em PSRAM + flash também é possível; imagens duplicadas nas
duas memórias não contam como fotos adicionais.

## Complemento: atualização por Wi-Fi

Usuário manifestou interesse em OTA na ESP32-CAM clássica e aceita apagar
as fotos ao entrar na atualização. Isso é uma preferência de projeto;
nenhum firmware ou partição foi alterado nesta conversa.

No OTA convencional, o novo aplicativo é gravado no slot inativo, validado,
e selecionado para o próximo boot. O outro slot preserva o aplicativo em
execução durante o recebimento. Voltar automaticamente à versão anterior
se a nova aplicação falhar exige configurar e implementar rollback.

Uma proposta compatível com os 4 MiB é dividir os 3 MiB já reservados ao
aplicativo em dois slots de 1,5 MiB e manter os 896 KiB de arquivos:

- app0: offset 0x10000, tamanho 0x180000;
- app1: offset 0x190000, tamanho 0x180000;
- arquivos: offset 0x310000, tamanho 0xE0000.

NVS, otadata, boot e coredump podem manter as áreas existentes. A proposta
depende de cada versão do firmware integrado caber em 1,5 MiB; o aplicativo
atual de 405.168 bytes é somente teste de câmera e não permite garantir isso.
Com esse layout, a capacidade de arquivos seria igual à atual. A proposta
anterior de cerca de 1,8 MiB para fotos com um único aplicativo não reserva
o segundo slot OTA.

Apagar fotos de uma partição de arquivos não aumenta os slots fixos do OTA.
Reutilizar o slot inativo para fotos exigiria armazenamento e controle
personalizados, apagamento antes do update e abrir mão de preservar ali a
versão anterior enquanto a área estiver sendo usada como fotos. Não é o
fluxo normal de LittleFS + OTA e não foi escolhido ou implementado.

O recebimento pode escrever em blocos, sem carregar o binário inteiro na
PSRAM. Câmera e fila de fotos podem ser liberadas no modo de atualização.
A primeira migração do layout e habilitação de OTA estão pendentes; o
firmware atual não implementa atualização por Wi-Fi.

Referência OTA:
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html

## Referências

- Esquema consultado: `outputs/ligacoes/ESP-CAM-esquema.html` e guia
  `ESQUEMA-GERAL-ESP-CAM-PCF8575.md` na mesma pasta.
- Fonte e manifesto: `firmware/camera_usb/serial_camera_usb.ino` e
  `firmware/release_esp32cam/manifest.json`.
- https://documentation.espressif.com/esp32_datasheet_en.html
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/external-ram.html
- https://github.com/espressif/esp32-camera

Os arquivos `HARDWARE.md` e `DECISOES.md`, referidos pelo índice/AGENTS.md,
não estavam presentes em `docs/conhecimento` nesta inspeção. Não foram
reconstruídos por inferência.
