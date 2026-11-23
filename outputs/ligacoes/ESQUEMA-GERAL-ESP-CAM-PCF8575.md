# Esquema geral — ESP32-CAM + PCF8575 + teclado 4×5 + GMG12864-06D

Proposta para o ESP32-CAM clássico identificado na COM17, com câmera e PSRAM
ativas, sem cartão microSD. Usar a placa azul apenas depois de substituir
o 4067 pelo PCF8575 no encapsulamento correto e confirmar continuidade.
Todos os nomes C0…C15/S0…S3/EN abaixo são os rótulos antigos dessa placa.

## ESP e alimentação

| Origem | Destino |
|---|---|
| ESP 3V3 | VCC da placa + VDD da tela |
| ESP GND | GND da placa + VSS da tela + C6 + C7 + C10 |
| ESP GPIO13 | C8 / SDA |
| ESP GPIO14 | C9 / SCL |

C6/C7/C10 são A2/A1/A0 do PCF, aterrados para endereço I²C de 7 bits 0x20.
SIG é INT e não será usado. A alimentação de 3,3 V do ESP depende da fonte
que já alimenta a placa; este desenho não é um circuito de carga de bateria.

## Tela

| Pino da tela | Destino | PCF quando aplicável |
|---|---|---|
| CS | S0 | P06 |
| RSE | S1 | P07 |
| RS | S2 | P11 |
| SCL | GPIO15 | — |
| SI | GPIO2 | — |
| VDD | 3V3 | — |
| VSS | GND | GND |

SCL da tela é SPI e vai ao GPIO15; não juntar ao SCL I²C da placa (C9).
A/K ficam desligados: sem luz de fundo. IC_* da fonte de caracteres não usados.

## Teclado

| Fio lógico do teclado | Rótulo da placa | PCF |
|---|---|---|
| L1 | C0 | P05 |
| L2 | C1 | P04 |
| L3 | C2 | P03 |
| L4 | C3 | P02 |
| COL1 | C4 | P01 |
| COL2 | C5 | P00 |
| COL3 | C11 | P17 |
| COL4 | C12 | P16 |
| COL5 | C13 | P15 |

A ordem física dos nove fios não foi medida. Identificar L1–L4 e COL1–COL5
por continuidade. Não há VCC/GND dedicado no teclado passivo: seus nove fios
vão somente às portas listadas. Para ler colunas, o firmware escreve 1 nessas
portas e usa as fontes fracas internas do PCF8575. Varrer uma linha LOW por vez.
Este plano considera uma tecla por vez, sem diodos individuais.

## Passivos

| Valor | Entre | E |
|---|---|---|
| 4,7 kΩ | C8 / GPIO13 | 3V3 |
| 4,7 kΩ | C9 / GPIO14 | 3V3 |
| 10 kΩ | S0 / CS da tela | 3V3 |
| 10 kΩ | S1 / RSE da tela | 3V3 |
| 10 kΩ | GPIO2 / SI da tela | GND |
| 100 nF | VCC da placa convertida | GND perto do CI |

Os resistores ligam o sinal ao trilho indicado, não entram em série no fio.
Se já houver capacitor adequado entre VCC/GND na placa, conservá-lo.
Não usar pull-down em RSE com PCF8575. Não confundir o pull-down de GPIO2
com os pull-ups de CS/RSE. S3, EN, C14 e C15 são portas livres, não ligar
EN ao GND. Conferir eventual resistor original de EN caso essa porta venha
a ser utilizada. GPIO12, GPIO4, GPIO0 e GPIO16 não são usados neste esquema;
TX/RX permanecem disponíveis.

## Bateria: divisor 2 × 470 kΩ

Usar GPIO33 / ADC1_CH5, que permite leitura com Wi-Fi ativo. Na ESP32-CAM
AI Thinker, esse GPIO aciona o LED vermelho e não sai no conector de pinos.
Remover/isolar o ramo desse LED e identificar por continuidade o pad que
realmente chega ao GPIO33. Não soldar no lado de 3V3; não usar o LED branco
de flash, que é GPIO4 / ADC2.

```text
BAT+ ── R1 470 kΩ ──┬── GPIO33 (ADC1_CH5)
                    ├── R2 470 kΩ ── BAT− / GND
                    └── Cbat 100 nF ─ BAT− / GND
```

Com bateria em 4,20 V, o ADC recebe 2,10 V; consumo máximo do divisor:
4,47 µA. Preferir resistores de 1%. O capacitor deve ficar perto do ADC:
a resistência equivalente de 235 kΩ torna a fonte de sinal de alta impedância.
Constante RC: 23,5 ms. No firmware, aguardar 200 ms após energizar/acordar,
ler amostras espaçadas e calcular média, usando ADC calibrado com atenuação
de 12 dB (11 dB em APIs antigas). VBAT = 2 × VADC; conferir com multímetro.
Configurar GPIO33 como entrada analógica, sem pulls internos, sem comando de LED.
Percentual por tensão é aproximado e depende da célula e da carga.

Conectar BAT+ à bateria real, antes do boost/regulador; BAT− e GND do ESP
devem estar unidos. O divisor não é circuito de alimentação nem carregador.
Não ligar os 4,2 V diretamente ao pino 3V3. Evitar manter BAT+ no divisor
com o ESP totalmente sem alimentação para não injetar corrente pela entrada;
se houver chave geral, o topo do divisor pode ficar após essa chave, ainda
antes do conversor de tensão, para desligar junto com o aparelho.

## Estado e fontes

Ligação planejada, sem ensaio de bancada de teclado/LCD. Em 09/11 foi instalada
a base `cam-ota-r2-wifi`, com fila de fotos, OTA e drivers preliminares PCF/LCD.
O teste físico foi sem periféricos; menu e mapa físico do teclado seguem pendentes.
Os três controles do LCD passam pelo PCF; clock e dados são gerados no ESP.
No core 3.3.6 desta bancada, o SCCB da câmera usa I2C1; o PCF usa TwoWire(0),
mantendo SDA=13/SCL=14. Essa separação foi conferida no SDK e no diagnóstico.
Manter uma imagem de
16 bits dos estados do PCF e serializar varredura/controle do LCD: atualizar a
tela não pode sobrescrever os bits de linhas/colunas, nem o contrário.

- PCF8575: https://www.ti.com/lit/ds/symlink/pcf8575.pdf
- CD74HC4067: https://www.ti.com/lit/ds/symlink/cd74hc4067.pdf
- Tela: https://ae-pic-a1.aliexpress-media.com/kf/S3dd8f051c7fc44a2873b1aee32568c34c.pdf
- ESP32 (GPIO2 / boot): https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
- ESP32-CAM (LED vermelho / GPIO33): https://raw.githubusercontent.com/SeeedDocument/forum_doc/master/reg/ESP32_CAM_V1.6.pdf
- ADC e Wi-Fi: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/peripherals/adc_oneshot.html
- ADC, calibração e capacitor: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/peripherals/adc_calibration.html
