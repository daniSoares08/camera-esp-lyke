# PCF8575 na placa do CD74HC4067

Conversao proposta por comparacao de datasheets, sem verificacao fisica da placa.

| Rotulo da placa | Pino fisico do CI | Funcao no PCF8575 |
|---|---:|---|
| C0 | 9 | P05 |
| C1 | 8 | P04 |
| C2 | 7 | P03 |
| C3 | 6 | P02 |
| C4 | 5 | P01 |
| C5 | 4 | P00 |
| C6 | 3 | A2 |
| C7 | 2 | A1 |
| C8 | 23 | SDA |
| C9 | 22 | SCL |
| C10 | 21 | A0 |
| C11 | 20 | P17 |
| C12 | 19 | P16 |
| C13 | 18 | P15 |
| C14 | 17 | P14 |
| C15 | 16 | P13 |
| SIG | 1 | INT |
| S0 | 10 | P06 |
| S1 | 11 | P07 |
| S2 | 14 | P11 |
| S3 | 13 | P10 |
| EN | 15 | P12 |
| VCC | 24 | VCC |
| GND | 12 | GND |

Alimentacao: VCC=3,3V, GND comum. C6/C7/C10 no GND: endereco I2C de 7 bits 0x20. C8=SDA/GPIO13; C9=SCL/GPIO14, cada um com pull-up de 4,7k para 3,3V. SIG e INT e pode ficar sem ligacao. Manter/adicionar 100nF entre VCC e GND.

Conferir passo/largura do encapsulamento e pino 1 antes de soldar. Se houver resistor ou ponte EN-GND na placa, conferir/remover para usar P12 livremente. Os demais sinais nao tem mais suas funcoes de multiplexador.

O PCF8575 possui saida HIGH fraca: nao reutilizar o pull-down de reset do LCD indicado para MCP23017. Firmware e mapa do teclado/LCD precisam ser adaptados ao PCF8575.

Datasheets: [CD74HC4067](https://www.ti.com/lit/ds/symlink/cd74hc4067.pdf), [PCF8575](https://www.ti.com/lit/ds/symlink/pcf8575.pdf).
