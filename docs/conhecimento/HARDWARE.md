# Hardware e pinagens

## Montagem Lyke em 22/11/2026

**Atualização de 23/11/2026:** STM32 R8 e ESP R5 operaram juntos, com
49 teclas/mapeamento completo recebidos pela UART. Captura JPEG e envio
à API com retorno e arquivo privado passaram. Nenhuma pinagem foi alterada
nesta etapa. A alimentação definitiva por bateria ainda requer aferição.

**Orientação de alimentação e terra, 22/11/2026 (ligação ainda a confirmar):**
alimentar a ESP32-CAM pelo pino **5V**, com retorno no **GND ao lado de 5V**.
Manter os pinos **3V3 e VCC sem alimentação externa** nessa configuração;
a última medição de VCC foi 3,3 V. Unir o GND do STM32 ao mesmo terra da
fonte/ESP. Não é necessário ligar todos os pinos de terra.

**Atenção à variante GND/R:** o terminal na ponta da fileira de U0T/U0R,
ao lado de U0T, pode ser RESET, apesar de parecer GND em outro pinout.
Não usá-lo como retorno de alimentação sem confirmar a variante; preferir
o GND junto de 5V. A variante da unidade física ainda não foi identificada.
A ligação desse terminal ao GND poderia manter uma variante GND/R em reset;
isso é uma hipótese para investigar, não diagnóstico da falha anterior.
O GND real junto de IO0 é outra referência de terra. Em uso normal, deixar
IO0 sem jumper ao GND. Fazer alterações com a alimentação desligada.
Referência primária para as variantes e alimentação:
[Prusa — ESP32-CAM, diferentes placas e ligação](https://github.com/prusa3d/Prusa-Firmware-ESP32-Cam/blob/master/doc/AI_Thinker-ESP32-cam/README.md#different-mcu-version).

**Medição mais recente, após reconectar o adaptador:** usuário informou
**VCC = 3,3 V, 3V3 = 3,3 V e 5V = 5,01 V**. Esses valores substituem
as leituras anteriores apenas para a condição atual; não comprovam a
seleção física de VCC nem estabilidade durante picos de consumo.
Após isolar o ESP no adaptador, respondeu ao reset automático em COM17.
Lyke R4 gravada e verificada em app0; boot/AP e OFF seguido de reset
foram testados. A montagem final com o STM ainda precisa ser restabelecida.

Alimentação: usuário confirmou fonte de 5 V ligada tanto a **5V** quanto
a **VCC** na ESP32-CAM, seguida de ausência de AP e flash branco fraco.
Solicitado retirar o fio externo de VCC com tudo desligado e alimentar
o ESP somente por 5V/GND. Correção e medições pendentes. No esquema
AI Thinker ESP32_CAM V1.6 de referência, VCC (P1, pino 5) tem seleção
para 3V3/5V por R1/R2 de 0 ohm; não presumir que seja uma segunda entrada
de 5 V. Não alimentar 3V3 com 5 V. A seleção física não foi confirmada.
Na sequência, usuário mediu VCC em 5 V e 3V3 em 3,24 V; não se confirmou
sobretensão na linha 3V3. Ausência de AP ainda sob investigação. Não registrar
dano nem VCC selecionado para 3V3 como diagnóstico desta unidade.

STM32G431CBU6 com firmware Lyke R8 e ESP32-CAM clássico AI Thinker
com Lyke R5. O teclado tem 49 contatos mapeados no STM32, ON dispensada,
T19 em PC6 após retirada do LED. O LCD GMG12864-06D Ver.2.2 usa
contraste 5, rotação 180° e 21 colunas nas seis linhas inferiores.
Pinagem completa em [TECLADO-19.md](TECLADO-19.md) e pares reais em
[teclado-lyke.csv](teclado-lyke.csv).

A ligação final usa PA2/A2 → GPIO13 e PA3/A3 ← GPIO14, com GND comum.
O usuário vai recolocar os fios nessa posição. A bateria continua com
divisor de 2 × 470 kΩ em PA0/A0, ainda sem teste de tensão aferido.
As ligações CLK → U0R e A3 ← U0T foram temporárias de diagnóstico.
O adaptador FTDI voltou a encaixar na ESP32-CAM conforme o arranjo original
informado pelo usuário; 3V3 ficou sem fio. O relato de VCC/5V deve ser
conferido no esquema da placa antes de reproduzir a alimentação.

Para o esquema atual da ESP32-CAM, consulte [teclado original de 19 trilhas](TECLADO-19.md).
As referências abaixo à matriz 4×5 descrevem a proposta anterior de 09/11.

Os números abaixo são GPIOs, não a posição física no conector. Identificação
real da placa e continuidade prevalecem sobre desenho de anúncio. Estado em
09/11/2026; ligações novas continuam propostas até teste físico.

## Inventário de componentes

| Componente | Identificação / função | Evidência e limite |
|---|---|---|
| Placa S3 original | GOOUUU ESP32-S3-CAM V1.5 / N16R8; 16 MB flash, 8 MB OPI PSRAM | Plataforma da calculadora e testes anteriores; conferir revisão física versus esquema publicado |
| Placa de tela original | ESP32 DevKit V1 / ESP32-WROOM-32, 4 MB flash | Recebe quadros UART da S3 e desenha LCD paralelo |
| Placa nova | ESP32-CAM perfil AI Thinker, ESP32-D0WD-V3 rev.3.1, 4 MB flash, 4 MB PSRAM | Identificação eletrônica; nome “ESP32-S3-CAM” do anúncio não corresponde ao chip medido |
| Sensor das câmeras | OmniVision OV5640, PID 0x5640 | JPEG QSXGA 2560×1920 confirmado; não presumir 2592×1944 no driver |
| Módulo AF anterior | DC5640-AF 78MM | Registrado nos testes S3 e migração |
| Módulo FF | DCXYX-LZTKQJ-5M-357-V1 FF | Foco fixo; não exigir sucesso AF na calculadora |
| Módulo AF recente | Desenho FD5640-S00W, OV5640, 24 vias, lente 2,7 mm F/2.8, campo nominal 66° | Desenho fornecido pelo usuário; nome da lente não prova identidade de todos os módulos anteriores |
| Tela original | Display Screen 75×52,7, S6B0107, LCM12864D-V1.0 WHITE, 128×64 | Interface paralela compatível KS0108; 20 pinos, 5 V, contraste VEE |
| Tela nova | GMG12864-06D Ver.2.2, ST7565R, 128×64 | SPI; usar CS/RSE/RS/SCL/SI/VDD/VSS; plano lógico 3,3 V |
| Tela de demo histórica | GM12864-59N Ver.2.0 IIC, ST7567S, 128×64 | I²C; não é a GMG nova; demo isolada na S3 usa SDA41/SCL42 |
| Teclado original | Placa cortada de HP48, matriz medida de 49 posições | 46 aceitas; M/apóstrofo, S/SIN e ALPHA não detectadas, perdas aceitas |
| Teclado novo | Matriz passiva 4×5, 20 teclas, 9 contatos | Modelo comercial não informado; ordem física do cabo ainda não medida |
| Placa azul | Módulo multiplexador analógico 16 canais CD74HC4067 | Usuário escreveu CD47HC4067; comparação usa o datasheet CD74HC4067 |
| CI substituto | TI PCF8575, marcação PF575, TSSOP-24 informado | Expansor I²C de 16 portas; não é multiplexador. Confirmar encapsulamento, orientação e continuidade antes de alimentar |
| Bateria | Uma célula Li-ion/LiPo. Foto com marcação `603040`, `3.7V`, `2000mAh`. Limite informado: 4,2 V | Capacidade apenas declarada na etiqueta. Correntes admissíveis, modelo da plaquinha e carregador/boost ainda sem confirmação |
| Divisor novo | R1/R2 470 kΩ; capacitor 100 nF | GPIO33 liberado do LED vermelho; ainda sem medição física |

## Modificação de alimentação da câmera AF recente

Usuário mediu aproximadamente 1,8 V no pino 23 do conector da câmera e
acrescentou fio de 3,3 V **sem isolar a ligação original**. Desenho fornecido
identifica pino 23 como AF-VDD (2,8–3,3 V), AVDD 2,45–2,80 V, DVDD 1,5 V,
DOVDD 1,8–3,3 V. São domínios diferentes: não aplicar essa alteração a outros
pinos de alimentação nem presumir que o esquema da outra câmera seja igual.
Não foi medida a corrente nem comprovada a origem dos 1,8 V. O usuário confirmou
essa montagem e pediu prosseguir; não registrar como modificação eletricamente
validada ou como instrução genérica para todas as placas.

## ESP32-CAM clássica: câmera e reservas

| Sinal | GPIO |
|---|---|
| D0/D1/D2/D3 | 5 / 18 / 19 / 21 |
| D4/D5/D6/D7 | 36 / 39 / 34 / 35 |
| XCLK / PCLK | 0 / 22 |
| VSYNC / HREF | 25 / 23 |
| SCCB SDA / SCL | 26 / 27 |
| PWDN / RESET | 32 / -1 |

GPIO16 é PSRAM, preservar. GPIO1/3 são UART. GPIO4 é o flash branco,
ADC2; GPIO33 é LED vermelho, ADC1_CH5, sem saída no conector padrão.

Foto adicional de 09/11: componente SOT-23 marcado aparentemente `A2SHB`
ao lado do LED branco. A marca `A2` é compatível com AO3402 (MOSFET N);
pela posição, provavelmente é a chave do flash branco. É inferência da foto
e da marca, não medição da placa/clonagem: o esquema V1.6 de referência
usa S8050 nesse ramo, controlado por GPIO4. Não indicar nenhum pad desse
componente como GPIO33. Removê-lo, se confirmado como chave do flash,
desabilita o flash branco e não libera o LED vermelho/GPIO33.

Alternativa à retirada do LED vermelho: interromper seu ramo retirando o
resistor em série, após identificá-lo. No esquema de referência, a ordem é
3V3 -> R4 (1 kΩ) -> LED vermelho -> GPIO33; portanto o pad de R4 no lado do
LED **não é um acesso direto ao GPIO33**, pois ainda existe o LED entre eles.
Só usar para solda um ponto com continuidade direta ao GPIO33, medido sem
USB/bateria. Depois do teste de piscadas do GPIO33 na revisão r3, o usuário
informou que dessoldou o LED vermelho. A retirada é confirmação do usuário;
pad do GPIO33, solda do fio e divisor ainda não têm confirmação por medição.
Na sequência, a revisão `cam-ota-r4-clean` removeu todo o teste de LED e foi
instalada por Wi-Fi em app1. GPIO33 inicia como entrada sem pull; ADC de
bateria permanece desabilitado até habilitação explícita.

MicroSD não será usado; seus sinais podem atender à tela/PCF. GPIO12 é strap
sensível à tensão da flash, por isso não foi selecionado. GPIO2 precisa ficar
baixo na entrada em download: há pull-down 10 kΩ no SI do LCD.

## Montagem nova: ESP → PCF8575 e tela

Todos os sinais lógicos em 3,3 V, GND comum. Rótulos C0…C15/S0…S3/EN/SIG
referem-se à **placa azul após a troca** do 4067 pelo PCF8575.

| Origem | Destino | Passivo |
|---|---|---|
| GPIO13 | C8 / SDA | 4,7 kΩ para 3V3 |
| GPIO14 | C9 / SCL I²C | 4,7 kΩ para 3V3 |
| 3V3 | VCC da placa e VDD da tela | 100 nF VCC–GND junto ao PCF; conservar o existente se adequado |
| GND | GND da placa, VSS da tela, C6/C7/C10 | A2/A1/A0 aterrados: endereço 7 bits 0x20 |
| S0 / P06 | CS da tela | 10 kΩ para 3V3 |
| S1 / P07 | RSE da tela | 10 kΩ para 3V3, **não GND** |
| S2 / P11 | RS da tela | — |
| GPIO15 | SCL SPI da tela | Separado de C9/SCL I²C |
| GPIO2 | SI da tela | 10 kΩ para GND |

A/K (backlight) e IC_* da tela ficam desconectados neste plano. SIG é INT,
não usado. Portas livres PCF: S3, EN, C14, C15. EN passa a ser P12: não aterrar.
O PCF é quase bidirecional: manter colunas escritas em 1 para ler e varrer uma
linha LOW por vez. Plano para uma tecla por vez, sem diodos individuais.

Firmware-base `cam-ota-r2-wifi`: PCF em TwoWire(0), GPIO13/14, separado do
SCCB da câmera em I2C1 no core 3.3.6, confirmado em SDK/diagnóstico. Imagem compartilhada
de 16 bits dos estados do PCF e exclusão mútua entre varredura e controles
do LCD. O clock/dado SPI ficam nativos no ESP, os três controles passam pelo
PCF, exigindo callback/driver apropriado; não usar driver MCP23017.

### Teclado novo

| Contato lógico | Placa azul | PCF |
|---|---|---|
| L1/L2/L3/L4 | C0/C1/C2/C3 | P05/P04/P03/P02 |
| COL1/COL2 | C4/C5 | P01/P00 |
| COL3/COL4/COL5 | C11/C12/C13 | P17/P16/P15 |

Não confundir COL1 do teclado com C1 na placa. Identificar os nove contatos
por continuidade. Não há fio de alimentação separado no teclado passivo.

### Conversão completa dos terminais

Pino 1 deve alinhar com pino 1 original. Alimentação física coincide: pino 24
VCC, pino 12 GND. Compatibilidade elétrica dos pinos de alimentação não garante
encaixe mecânico de SSOP/TSSOP. A rota do módulo deve ser conferida fisicamente.

| Rótulo antigo | Função PCF8575 |
|---|---|
| C0/C1/C2/C3/C4/C5 | P05/P04/P03/P02/P01/P00 |
| C6/C7 | A2/A1 |
| C8/C9/C10 | SDA/SCL/A0 |
| C11/C12/C13/C14/C15 | P17/P16/P15/P14/P13 |
| SIG | INT |
| S0/S1/S2/S3 | P06/P07/P11/P10 |
| EN | P12 |
| VCC/GND | VCC/GND |

Detalhamento pino físico ↔ terminal está em
`outputs/ligacoes/PCF8575-no-modulo-4067.json`.

## Bateria: monitor novo no GPIO33

```text
BAT+ ── R1 470 kΩ ──┬── GPIO33 / ADC1_CH5
                    ├── R2 470 kΩ ── BAT− / GND
                    └── Cbat 100 nF ─ BAT− / GND
```

- GPIO33/ADC1 é a escolha para funcionar junto com Wi-Fi. Retirar/isolar o
  ramo do LED vermelho; soldar no pad que chega ao GPIO33, identificado por
  continuidade, **não no lado de 3V3**. Não trocar pelo flash branco GPIO4.
- 4,20 V → 2,10 V; corrente do divisor 4,47 µA. Resistores preferencialmente 1%.
- Impedância equivalente 235 kΩ: capacitor perto do ADC; RC 23,5 ms.
- Firmware: entrada analógica, pulls internos desligados; não acionar como LED.
  ADC calibrado com atenuação 12 dB (11 dB nas APIs antigas), aguardar ~200 ms
  ao ligar/acordar, média de amostras espaçadas; VBAT = 2 × VADC. Conferir
  com multímetro. Não converter raw ADC como se 4095 fosse exatamente 3,3 V.
- Percentual por tensão é aproximado, depende da célula e da carga.
- Medir antes do boost/regulador, com BAT− e GND unidos. Se há chave geral,
  topo do divisor após a chave e antes do conversor desliga junto do aparelho
  e evita injeção pela entrada com o ESP sem alimentação. Esse esquema mede
  tensão, não é fonte/carregador; não aplicar 4,2 V diretamente no 3V3.

## Calculadora antiga: S3 → DevKit → LCD paralelo

S3 TX43 → DevKit RX2/GPIO16; GND comum. UART 115200 8N1, unidirecional.
DevKit TX17 fica sem fio; S3 RX44 para USB/UART CH343. Quadros com sincronismo,
sequência e CRC16; repetição periódica permite recuperação após boot.
GPIO19/20 da S3 são teclado: manter OTG desconectada nessa montagem.

| Pino LCD LCM12864D | Sinal | Ligação |
|---:|---|---|
| 1/2 | VDD/VSS | +5 V regulados / GND |
| 3 | VO | Meio do divisor de contraste |
| 4/5/6/7 | DB0/DB1/DB2/DB3 | GPIO4/14/13/18 |
| 8/9/10/11 | DB4/DB5/DB6/DB7 | GPIO19/21/22/23 |
| 12/13/14 | CS1/CS2/RST | GPIO25/26/27 |
| 15 | RW | GND permanente |
| 16/17 | RS/E | GPIO32/33 |
| 18 | VEE | Saída negativa de contraste, não GND |
| 19/20 | LEDA/LEDK | +5 V por 220 Ω / GND, ou backlight desconectado |

Tela montada de ponta-cabeça; rotação global 180° no DevKit. CS ativos LOW,
tempos de barramento/reset ajustados; texto e metades apareceram corretamente
no teste físico. Serial de diagnóstico 115200: t teste, n normal, r reset,
s contadores; RW em GND impede leitura de confirmação do LCD.

### Contraste: decisão posterior substitui valores iniciais do guia antigo

Potenciômetro medido **fora do circuito**: +5V→meio 3,33 kΩ; total +5V→VEE
10,41 kΩ; meio→VEE 7,23 kΩ. Usuário tem 3,3 kΩ e 6,8 kΩ: substituição aproximada
solicitada é **+5V → 3,3 kΩ → VO → 6,8 kΩ → VEE**. Não confundir com o resistor
de 220 Ω da luz de fundo. O contraste pode exigir ajuste físico; os 15 kΩ/5,1 kΩ
do LEIA-ME antigo eram um ponto inicial anterior à medição, não a escolha final.

### Teclado HP48

Mapa autoritativo: `firmware/calculadora/common/keyboard_map.h`, reproduzido
em `teclado-hp48.csv` desta base. 49 posições, 46 contatos e três perdas aceitas.
GPIOs foram ligados aleatoriamente; não reconstruir matriz por suposição.
Mapeador: `teclado/server.py`, `teclado/index.html`, `firmware/mapear_teclado`;
iniciador `Abrir mapeador do teclado.bat`, serviço histórico porta 8766.
O usuário cortou a placa original e restaram 3–4 componentes; não houve
confirmação de remoção desses componentes. Oscilações/contatos múltiplos
ocorreram durante o mapeamento, e alguns filtros/varredura foram corrigidos.

## Energia, LEDs e USB

Para a montagem STM32G431 + ESP32-CAM, ver a [proposta de alimentação](TECLADO-19.md#alimentação-e-recarga-da-montagem-stm32--esp32-cam).

Bateria 1S não gera 5 V por si: tela paralela antiga precisa de boost regulado.
Modelos concretos de boost/carregador não foram identificados. Unir GNDs;
não presumir seleção segura entre alimentação externa e USB.

Na S3, RGB GPIO48 recebe tentativa de preto, depois é devolvido ao teclado;
não forçar HIGH se estiver preso em LOW. DevKit LED GPIO2 mantido LOW.
LEDs de alimentação/TX não possuem desligamento independente pelo software
nessas revisões; remoção exige identificar ramo correto. No ESP32-CAM novo,
GPIO33 será ADC após liberação do LED vermelho; GPIO4 é flash branco.

Usuário cogitou extensão dos conectores USB para fora da carcaça. Isso não
transforma TX/RX em D+/D−: USB/UART passa pelo conversor. Na S3 GPIO19/20 são
USB nativo, já ocupados pelo teclado antigo. Para extensão do USB do conversor,
trabalhar nos sinais do conector/conversor e preservar requisitos do USB-C;
não há pinagem de solda física final confirmada nesta base.

## Fontes locais e datasheets

- Placa GOOUUU: `dados/laboratorio/GOOUUU_schematic.*`, `outputs/GOOUUU_esquema_publicado.pdf`.
- LCD antiga: `firmware/demo_lcm12864/especificacao_vendedor.png` e guias dessa pasta.
- PCF8575: https://www.ti.com/lit/ds/symlink/pcf8575.pdf
- CD74HC4067: https://www.ti.com/lit/ds/symlink/cd74hc4067.pdf
- ESP32: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
- ESP32-CAM: https://raw.githubusercontent.com/SeeedDocument/forum_doc/master/reg/ESP32_CAM_V1.6.pdf
- ADC: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/peripherals/adc_oneshot.html
- Calibração/filtro: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/peripherals/adc_calibration.html
- AF OV5640: https://usermanual.wiki/Document/OV5640FirmwareUserGuideV10.952852672.pdf
- LCD nova: fonte e detalhes no guia `outputs/ligacoes/ESQUEMA-GERAL-ESP-CAM-PCF8575.md`.
