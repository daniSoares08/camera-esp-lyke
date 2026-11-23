# Teste da tela LCM12864D-V1.0

Demonstração para ESP32-S3 e LCM12864D-V1.0 WHITE, com controlador compatível
com KS0108/S6B0108, tela 128×64 e barramento paralelo de 8 bits.
Versão v2: alimentação e polaridade de CS corrigidas conforme a primeira
folha técnica enviada pelo usuário, correspondente ao LCD adquirido.
Versão v4: ordem visual confirmada com radar parado e CS ativo LOW:
bloco esquerdo em CS1/GPIO12 e direito em CS2/GPIO13 (`swap=0`).
A v3 aplicava uma troca extra de metades e foi substituída pela v4.
Não inicializa câmera nem Wi-Fi. Funciona sem computador depois de gravada.

## Uso

- Troca automática de cena a cada 12 segundos, em ciclo contínuo.
- BOOT curto: próxima cena.
- BOOT pressionado por 0,8 segundo: pausar/continuar.
- Contraste ajustado fisicamente no potenciômetro; não existe ajuste digital
  de contraste neste driver. O firmware não mede a iluminação.

As cenas são: robô que pisca e acena, radar, cubo 3D, viagem pelas estrelas,
Pong automático, ondas, equalizador, relógio ilustrativo, teste de pixels e
teste das duas metades. Radar, áudio, ondas e relógio são simulações.

O teste de pixels inclui apagado/aceso, xadrez de 1 e 4 pixels, linhas
verticais/horizontais, borda com diagonais e varredura. Cada padrão dura 1,5 s.
O teste de metades identifica CS1/esquerda e CS2/direita, percorre as oito
páginas e faz uma bolinha cruzar a junção.

## Fios

| Sinal LCD | Conexão |
|---|---|
| 1 VDD | 5 V da placa, conforme a folha técnica da versão WHITE |
| 2 VSS | GND |
| 3 VO | Cursor do potenciômetro |
| 4–11 DB0–DB7 | GPIO4–GPIO11, na mesma ordem |
| 12 CS1 | GPIO12 |
| 13 CS2 | GPIO13 |
| 14 RST | GPIO14 |
| 15 RW | GND |
| 16 RS | GPIO15 |
| 17 E | GPIO16 |
| 18 VEE | Extremidade do potenciômetro, conforme o print |
| 19 BA | 5 V através do resistor de 220 Ω informado |
| 20 BK | GND |

Use o potenciômetro de 20 kΩ: extremidades em VDD (+5 V) e VEE (pino 18),
cursor central em VO (pino 3). Ajuste lentamente após ligar.
O usuário confirmou que a câmera está desconectada do conector flat.

## Especificação agora identificada

Produto informado: LCM12864D-V1.0 WHITE, neste
[anúncio](https://pt.aliexpress.com/item/1005003578380852.html).
A página não abriu na ferramenta web; os dados abaixo foram lidos diretamente
da primeira imagem técnica enviada pelo usuário. Cópia em `especificacao_vendedor.png`.

- Alimentação lógica VDD: 5 V ±0,25 V; a ligação inicial em 3,3 V não era nominal.
- CS1 e CS2: ativos em nível baixo; v2 inicia com `csLow=1`.
- Entrada HIGH mínima de 2,0 V e LOW máxima de 0,8 V: sinais de saída de
  3,3 V do ESP são compatíveis com os níveis informados pelo vendedor.
- VOUT/VEE, pino 18: saída negativa indicada como −10 V. Não é entrada
  de alimentação nem terra e não deve ser ligada aos GPIOs.
- Tensão típica de acionamento VDD−VO: 11,2 V. Com VDD=5 V, corresponde
  a VO próximo de −6,2 V; o ajuste real depende do painel e da temperatura.
- RW deve permanecer no GND: o barramento é usado apenas para escrever,
  evitando ler sinais de 5 V de volta para o ESP.
- O resistor de 220 Ω limita a corrente do backlight, que pode ficar menos
  brilhante do que no ponto típico de 45 mA indicado na folha.

As outras imagens com título LEG128642-3.3V, controlador SSD1309Z e OLED
referem-se a outro produto; suas tensões, pinagem e driver não se aplicam
ao LCD LCM12864D-V1.0 WHITE fotografado pelo usuário.

## Monitor USB OTG, 115200

| Comando | Ação |
|---|---|
| `1`…`9`, `0` | Fixar cena 1…9 ou 10 |
| `n` | Avançar cena |
| `a` | Alternar troca automática |
| `p` | Alternar pausa |
| `i` | Inverter pixels |
| `r` | Girar 180° |
| `c` | Alternar polaridade de CS e reinicializar LCD |
| `w` | Trocar as metades e reinicializar LCD |
| `d` | Reinicializar LCD |
| `s` | Estado, quadros, hash do buffer e tempo de transferência |
| `b` | Buffer enviado, 1024 bytes em hexadecimal |
| `h` | Ajuda |

Os ajustes voltam ao padrão após RESET. Padrão na v4: CS ativo baixo e
metades na ordem do driver (`csLow=1 swap=0`).
Com RW no GND, a comunicação é somente de escrita: a serial confirma a
execução do programa, mas não detecta presença nem confirma imagem no LCD.

## Compilar e gravar novamente

Execute `Compilar.ps1` nesta pasta. Dependências deste computador: Arduino
CLI 1.5.1, core esp32 3.3.6 e U8g2 2.36.19. A configuração usa flash de
16 MB, PSRAM OPI e USB OTG/TinyUSB com CDC habilitado no boot.

Os componentes e respectivos hashes ficam em `bin/manifest.json`.
A gravação usa bootloader em 0x0, partições em 0x8000, boot_app0 em 0xE000
e aplicação em 0x10000. Substitui o firmware anterior.

Referência do construtor e da ordem dos pinos:
[U8g2, documentação oficial](https://github.com/olikraus/u8g2/wiki/u8g2setupcpp).
