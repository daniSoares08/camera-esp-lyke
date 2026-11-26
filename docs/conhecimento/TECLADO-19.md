# Teclado original de 19 trilhas — 15/11/2026

## Estado em 26/11/2026: R14 final instalada e MODE confirmado

MAP49 e prontidão conferidos após OTA final STM R14/ESP R12. Usuário testou
SHIFT+AC, tecla numérica sem despertar e somente MODE despertando com menu
funcional. Produção usa PB12/PB9 em STOP1 e PA2→GPIO13 para acordar o ESP.
Versões temporárias com PA3 foram removidas; sem alteração de fios/mapa.

## Estado de 25/11: R14 preparada, diagnóstico instalado

MAP49 conferido na dupla R14-test/R12-test, sem alterar o mapa. Os fontes finais
R14 usam MODE PB12/PB9 para sair de STOP1. A versão temporária de ensaio usa
PA3 e não foi aprovada no ciclo completo; restaurar produção assim que o portal
for recuperado com ciclo pela chave. Ver [R14](../REVISAO-R14-TEMPORIZADORES.md).

O usuário informou 19 trilhas entre o teclado original da nova calculadora
e a placa. O mapa concluído de 49 contatos foi extraído da flash do STM32
em [teclado-lyke.csv](teclado-lyke.csv); ON foi dispensada.
O [HTML de montagem](../../outputs/ligacoes/ESP-CAM-esquema.html) mostra a
arquitetura com STM32 e ESP32-CAM clássica.

## Sono preparado em 25/11/2026

STM R13 + ESP R11 ainda não gravados: MODE usa PB12/T02 e PB9/T16,
conforme o mapa real. Em STOP1 só esse par tem pull-up/saída baixa e EXTI;
as demais trilhas ficam analógicas sem pulls. Ao despertar, o STM restaura
a matriz e acorda o ESP pela UART PA2→GPIO13 existente. Nenhum fio adicional.
[Protocolo, limites e aplicação](../REVISAO-R13-SONO-PROFUNDO.md).

## Alternativa com STM32G431CBU6 e ESP32-CAM

O usuário possui uma placa STM32G431CBU6 com layout compatível com a WeAct
Core Board QFN48. A divisão recomendada usa o STM32 para teclado, LCD
GMG12864-06D, medição da bateria, menus e cálculo científico offline.
A ESP32-CAM cuida da câmera, JPEG/PSRAM, Wi-Fi, API/HTTPS e OTA. A comunicação
pode usar UART de 3,3 V, 115200 8N1, com GND comum. O mapa interativo usa
`outputs/ligacoes/stm32-esp32cam.json` e `esquema-stm32.template.html`. Para
regenerar o HTML offline, execute `python outputs/ligacoes/gerar_html.py`.

| Função | Ligação proposta |
|---|---|
| T01–T16, borda inferior B13/B12 | PB13, PB12, PB15, PB14, PA8, PA10, PA9, PA15, PC10, PC11, PB3, PB5, PB4, PB7, PB6, PB9 |
| T17–T18, borda superior B11/B10 | PB11, PB10 |
| T19, borda inferior | PC6, com LED de usuário D1 removido |
| LCD CS, RSE e RS | PA4, PA6 e PA1 (GPIO) |
| LCD SCL e SI | PA5 (SPI1_SCK) e PA7 (SPI1_MOSI), AF5 |
| LCD VDD e VSS | 3V3 do STM32 e GND |
| ESP TX para STM32 RX | GPIO14 → PA3 (`A3`, USART2_RX, AF7) |
| STM32 TX para ESP RX | PA2 (`A2`, USART2_TX, AF7) → GPIO13 |
| Medição da bateria | PA0 (`A0`, ADC1 canal 1), na junção do divisor |
| GND | GND do STM32, ESP e LCD em comum |

Os números T01–T19 continuam identificadores de fios, com pares de teclas
a medir. Na orientação da foto, com USB-C à esquerda e SWD à direita, são 17
trilhas na borda inferior e 2 perto do USB-C na borda superior. O plano pressupõe
teclado isolado da CS6067-002. Para varrer, liberar os GPIOs com pull-up e
dirigir somente a trilha em teste em LOW, uma por vez. Confirmar SB3/SB5/SB6/SB7
abertas antes de ligar PA9, PA10, PB4 e PB6. Configurar `UCPD1_DBDIS = 1` para
desativar os pull-downs internos de PB4/PB6 antes da varredura. Reservar PA11/PA12
para USB, PA13/PA14 para SWD e PB8 para BOOT0. O usuário confirmou retirada
do LED de usuário e a ligação em PC6. Usar SWD de dois fios com SWO desativado,
configurando PA15, PB3 e PB4 como GPIO.
No SPI1, usar seleção por software e manter PA6 como GPIO de reset do LCD.
[Datasheet STM32G431CB](https://www.st.com/resource/en/datasheet/stm32g431cb.pdf)
e [documentação da placa](https://docs.zephyrproject.org/latest/boards/weact/stm32g431_core/doc/index.html).

A proposta anterior concentrava 13 fios na borda superior (PB11, PB10, PB2,
PB1, PB0, PC4, PA7, PA6, PA5, PA4, PA3, PA2, PA1) e 6 na inferior (PA15, PB3,
PB5, PB7, PB6, PB9). Usava SPI2 em PB13/PB15, CS em PB12, RSE em PB14, RS em
PA8 e USART3 em PC10/PC11. A revisão B13/B12 substitui esse plano de bancada.

Transferir o divisor para PA0 na variante STM32. Ligar 470 kΩ de BAT+ à
junção e 470 kΩ da junção ao GND do STM32, comum a BAT−. Ligar 100 nF entre
PA0 e GND perto do STM32. A junção recebe 2,10 V quando BAT+ = 4,20 V e o
divisor consome 4,47 µA. Se houver chave geral, ligar o divisor após a chave
e antes do regulador para evitar manter o ADC energizado com o STM32 desligado.
Configurar PA0 como analógico sem pulls. Calibrar ADC1, medir a referência
com VREFINT, aguardar aproximadamente 200 ms após energizar o divisor, usar
amostragem longa e espaçada e conferir com multímetro. Com 235 kΩ de
impedância equivalente, corrente de fuga e carga de amostragem podem alterar
o resultado. O STM32 pode enviar a tensão em mV ao ESP pela UART.
[Recomendações da ST para o ADC do STM32G4](https://www.st.com/content/st_com/en/technical-documents/an5346-stm32g4-adc-use-tips-and-recommendations-stmicroelectronics.html).

Na bancada, alimentar cada placa pela entrada de alimentação prevista ou
pelo USB da própria interface e unir os GNDs. Usar 3,3 V no LCD. Evitar unir
as saídas dos dois reguladores de 3,3 V e evitar aplicar a célula de 4,2 V
a um pino lógico. Para a alimentação definitiva, confirmar a variante da placa
e o caminho de VCC/V+ no [esquema WeAct](https://github.com/WeActStudio/WeActStudio.STM32G431CoreBoard/blob/master/Hardware/QFN48/WeAct-STM32G431CxUxCoreBoard_V10_SchDoc.pdf).

### Alimentação e recarga da montagem STM32 + ESP32-CAM

A foto mostra `603040`, `3.7V` e `2000mAh`. O usuário informou limite de
4,2 V. A ficha exata e as correntes de carga/descarga não foram confirmadas.
A plaquinha sob a fita parece ser de proteção. Usar carregador Li-ion/LiPo
1S de 4,20 V, CC/CV com término, alimentado com 5 V na entrada e corrente
ajustada à célula. Para usar durante a carga, prever saída de sistema com
power-path, como no [BQ24074](https://www.ti.com/product/BQ24074).

Na WeAct de referência, VCC entra no regulador de 3,3 V e pode receber 4,2 V.
Com a descarga, o regulador pode sair de regulação. Na ESP32-CAM AI Thinker,
a alimentação padrão é 5 V, conforme o
[esquema da placa](https://docs.ai-thinker.com/_media/esp32/docs/esp32_cam_sch.pdf).
Proposta de bancada: boost regulado em 5 V para VCC da WeAct e o pino `5V`
da ESP32-CAM, com GND comum. Dimensionar o boost, a célula e sua proteção
para os picos de consumo, e desligar o boost das placas antes de alimentá-las
por USB. Divisor do ADC permanece na tensão da célula. Montagem a testar.

A placa doadora da caixa de fones tem serigrafia `QL-1350 V1.0`, data
`2022-02-28` e CI `SY7658`. O [fabricante](https://www.tkplusemi.com/en/product/86.html)
indica carga 1S de 4,20 V, versões de 200/500 mA e boost de até 500 mA.
Conservar SW1 se o boost for usado até identificar sua variante. O manual
do CI permite deixar KEY em aberto quando dispensado. Nas fotos, o trecho
abaixo dos dois furos acima de SW1 é candidato a corte dos indicadores. A
continuidade das vias e planos, a variante do CI e o funcionamento sem SW1
ainda precisam de verificação. Nenhum corte ou ligação da nova célula confirmado.

O usuário mediu a queda de 5 V para 3,8 V na saída após desconectar o USB da
QL-1350. A carga presente nessa medição não foi informada. O comportamento
é compatível com variantes K do SY7658, que deixam VOUT fracamente puxado
para VBAT em espera e desligam o boost após cerca de 16 s abaixo de 9 mA.
Proposta em teste: chave em série entre L+ e a alimentação das placas,
mantendo SW1 para acordar o boost. Se confirmar desligamento por baixa
carga, testar 330 Ω de pelo menos ¼ W entre o positivo após a chave e L−,
consumindo cerca de 15 mA em 5 V. KEY não deve ficar aterrado continuamente.
O usuário prefere uma saída contínua sem carga artificial. A ficha Rev. 1.4
do SY7658 atribui esse comportamento às variantes L (por exemplo, E4L5R,
60 µA típicos sem carga, corrente de recarga de 500 mA); não documenta uma
configuração externa para transformar a variante K em L. Outra opção em
estudo é manter a QL-1350 para recarga e usar um boost de baixo consumo
com saída regulada em baixa carga, separado de L+/R+. Nada instalado.

Proposta para usar o USB-C da WeAct para carregar: tirar a alimentação do
VBUS bruto, no lado do conector do diodo D4, e levar a VIN da QL-1350 com GND
comum. No esquema WeAct, D4 conduz de VBUS para VCC e a ponte SB10 o contorna.
Confirmar SB10 aberta e identificar VBUS na placa com multímetro. O rótulo
V+ corresponde a VREF+, não ao barramento USB. VCC pode receber L+ do boost,
portanto não levar VCC a VIN. Se unir VBUS e VIN, alimentar por um único cabo
de carga, usar fonte de 5 V com corrente suficiente e desligar o ramo de
carga ao usar USB do computador para DFU. Proposta ainda sem teste físico.

### Firmware integrado

Para a ligação definitiva, usar A2/PA2 → GPIO13, A3/PA3 ← GPIO14 e
GND comum. CLK/PA14 → U0R e A3 ← U0T eram ligações de diagnóstico.
O usuário vai restabelecer a montagem final; validação da UART pendente.

Firmware integrado [Lyke](../../firmware/lyke_stm32/LEIA-ME.md) gravado
e conferido no STM32 (R8) e ESP (R3), em validação física da UART.
O portal abriu automaticamente no ESP após a atualização. O
[mapeador de bancada R4](../../firmware/stm32_teclado/LEIA-ME.md)
foi gravado com varredura de 19 trilhas, resultado no LCD e contraste 5
fixo. Todas as teclas apresentaram alguma resposta no teste informal da R3.
O mapeamento em ordem ainda é necessário para associar os pares às teclas.
O STM32 varre as trilhas, faz debounce,
interpreta teclas, calcula e desenha localmente. O ESP recebe comandos de foto,
consulta e OTA e retorna estado, progresso e trechos de texto paginados.
Durante o mapeamento, o portal Wi-Fi recebe a contagem de teclas; os pares
ficam gravados no STM32.
Um quadro monocromático tem 1024 bytes e levaria cerca de 89 ms pela UART
a 115200 8N1. O desenho local elimina essa transferência a cada tecla.
Comandos são linhas ASCII limitadas. A atualização usa blocos de 256 bytes
com confirmação e timeout e CRC32 do pacote. No perfil Lyke, GPIO13/14
foram liberados do antigo I²C do PCF para a UART remapeada. A UART0 em
GPIO1/3 fica disponível na bancada.

### Memória, desempenho e energia

- STM32G431CBU6: 128 KiB de flash, 32 KiB de RAM (22 KiB SRAM + 10 KiB CCM),
  Cortex-M4 até 170 MHz e FPU de precisão simples. A calculadora atual usa
  `double` e precisa de operações de 64 bits em software no STM32. Preservar
  precisão e verificar tempo/flash/RAM após portar `calculator_core.h`.
- ESP32-CAM identificada: dual-core até 240 MHz, 520 KiB SRAM nominal do chip,
  4 MiB de flash e 4 MiB PSRAM. Slots OTA de 1,5 MiB. Fotos e buffers de
  rede ficam no ESP. A UI antiga chega a copiar 12.000 caracteres de resposta
  e montar outra representação em linhas. No STM32, limitar a memória de
  texto, pilha e histórico e usar páginas. O STM32 pode manter 1 KiB de
  framebuffer local. Conferir mapa de link e pico de RAM no pior caso.
- Referências de chip em condições de datasheet: STM32 com CoreMark ~2,5 mA
  a 26 MHz em Range 2 e ~24 mA a 170 MHz em Boost, periféricos desligados.
  ESP32 dual-core em modem-sleep ~20–31 mA a 80 MHz e ~30–68 mA a 240 MHz.
  TX Wi-Fi ~180–240 mA nas condições de rádio listadas. LDO, LED, câmera,
  LCD, PSRAM e conversor da bateria influenciam a corrente da montagem.
  Esses valores não são medições das placas do usuário.
- Começar com rádio e câmera desligados fora do uso e medir a corrente em
  repouso. Para reduzir ainda mais, testar deep-sleep do ESP com EXT1 no
  próprio GPIO13 (entrada UART e RTC GPIO), `ALL_LOW` com um único pino.
  O TX do STM fica em HIGH no repouso, força LOW para despertar o ESP e
  retorna à UART após sinal de pronto, com timeout. O ESP reconfigura GPIO13
  para RX após acordar. O acionamento é por nível do RTC. Deep-sleep reinicia
  o ESP e perde JPEGs em PSRAM. Manter acordado com fotos pendentes ou usar
  armazenamento persistente dimensionado. Medir no conjunto antes de estimar
  autonomia. Uma futura chave de alimentação pode exigir outro fio de controle.

Referências de [memória e corrente STM32](https://www.st.com/resource/en/datasheet/stm32g431cb.pdf),
[corrente e GPIO do ESP32](https://documentation.espressif.com/esp32_datasheet_en.html)
e [EXT1 no ESP32](https://docs.espressif.com/projects/esp-idf/en/release-v5.3/esp32/api-reference/system/sleep_modes.html).

O STM32G431 tem USB DFU no bootloader de fábrica. Testar primeiro com cabo
USB de dados e placa sozinha. Segurar BOOT0, apertar e soltar NRST e soltar
BOOT0. Usar a interface USB do STM32CubeProgrammer. Se USB-C para USB-C não
enumerar, tentar USB-A para USB-C. SWD nos pinos CLK/DIO fica disponível para
depuração e recuperação. A entrada no DFU nesta unidade ainda não foi testada.
O OTA atual atualiza o ESP. Atualizar o STM32 por Wi-Fi através do ESP exigiria
implementar transferência, entrada no bootloader, verificação e recuperação.
A ROM do STM32G431 suporta USART2 nos pinos PA2/PA3 propostos.
[AN2606](https://www.st.com/resource/en/application_note/an2606-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics.pdf).

## Ligação anterior com dois PCF8575

São dois PCF8575 de 16 portas para acomodar até 19 sinais do teclado e três
da tela. As duas placas partilham I²C0, SDA GPIO13 e SCL GPIO14, com 3V3 e
GND comuns, um par de pull-ups de 4,7 kΩ e 100 nF local por CI. O barramento
SCCB da câmera mantém I²C1 no SDK usado pelo firmware atual.
Se a medição reduzir o teclado a 13 sinais independentes ou menos, é
possível revisar a pinagem para usar um único PCF8575 com a tela.

| Placa | Endereço | A2 / C6 | A1 / C7 | A0 / C10 | Teclado |
|---|---|---|---|---|---|
| A | 0x20 | GND | GND | GND | T01–T06 em C0–C5 |
| B | 0x21 | GND | GND | 3V3 | T07–T19 em C0–C5, C11–C15, S0, S1 |

C8/SDA, C9/SCL e nomes C/S pressupõem a placa azul com PCF8575 já no lugar
do CD74HC4067. Confira continuidade, footprint e componentes remanescentes.
Em módulo PCF8575 pronto, siga os nomes SDA/SCL, A0–A2 e as portas Pxx da
tabela do HTML. Confira e desfaça eventual strap A0–GND na placa B antes de
ligar A0 a 3V3. Referência de CI no [datasheet PCF8575](https://www.ti.com/lit/ds/symlink/pcf8575.pdf).

A tela GMG12864-06D mantém CS em S0/P06, RSE em S1/P07 e RS em S2/P11 da
placa A. SCL SPI está no GPIO15 e SI no GPIO2. O divisor 2 × 470 kΩ e 100 nF
continua no GPIO33. O usuário já informou a remoção do LED vermelho.

O plano completo trilha → placa → porta está em
[`teclado-19.json`](../../outputs/ligacoes/teclado-19.json), com a conversão
C/S → Pxx em `outputs/ligacoes/PCF8575-no-modulo-4067.json`. Esses arquivos,
o template antigo e o pôster 4×5 preservam as propostas anteriores.

## Identificação dos contatos

A foto da placa original mostra a serigrafia `CS6067-002` e a marca `1915`.
A pesquisa pelo código completo e por variantes não localizou mapa público
confirmado dessa revisão. O [registro chinês de circuito integrado](https://www.cnipa.gov.cn/art/2021/6/18/art_164_160107.html)
lista `CS6067` sob o número `BS.135014271`, ligado à China Resources Micro.
A coincidência do prefixo é uma pista. O registro não associa explicitamente
o circuito à revisão `-002` nem dá a função dos contatos desta placa.
Foram encontrados mapas de outras calculadoras. O [HACKACALC](https://hackaday.io/project/27920-hackacalc/log/69906-tracing-keyboard-matrix),
por exemplo, mapeia um clone de fx-82MS com 16 posições e uma via sem conexão.
A correspondência desses mapas com a `CS6067-002` continua indeterminada.

Para decidir o número de portas, contar as redes distintas usadas pelas
teclas que se quer manter. A fila da esquerda na foto é o lado a conferir
por continuidade até o teclado. A fila fina à direita parece seguir para o
LCD. A imagem não permite atribuir função a cada via nem validar que as 19
vias relatadas são independentes. Medir com o teclado isolado e sem energia.
Identificar redes já unidas em repouso, medir em ohms enquanto se pressiona
cada tecla e testar ON separadamente. Um contato resistivo pode ficar acima
do limiar do bip de continuidade. Não juntar vias diferentes pelo nome de
linha ou coluna. Só retirar uma via da ligação após verificar seu papel no
mapa de todas as teclas desejadas.

Numere T01–T19 a partir de um lado do cabo e marque a orientação. Com tudo
desligado, separe o teclado da eletrônica ativa original e procure vias de
alimentação, terra, curtos permanentes, diodos e pares que fecham ao pressionar.
A ligação proposta assume contatos passivos isolados. Se houver circuitos
ativos ou vias com outras funções, classifique essas vias antes de ligar.

Para levantar o mapa, liberar as vias de leitura, baixar uma por vez e
registrar quais baixam junto quando cada tecla é pressionada. Registrar
direção se houver diodos e marcar contatos ambíguos. O mapa da HP48 antiga
se refere à montagem anterior.

## Alternativa com um PCF8575

O usuário prefere montar com um único PCF8575 e a ESP32-CAM. O expansor
tem 16 portas. Com as ligações atuais da tela, sobram 13 portas para o
teclado. Se houver 19 redes distintas, uma alternativa é usar as 16 portas
do PCF, mais três GPIOs nativos, e compartilhar RS da tela com uma trilha
passiva. A alocação abaixo é uma proposta a validar em bancada.

| Sinal | Ligação proposta |
|---|---|
| SDA / SCL I²C | GPIO13 / GPIO14, PCF em `0x20` |
| 16 redes do teclado | P00–P07 e P10–P17 do PCF |
| Outras 3 redes do teclado | GPIO1, GPIO3 e GPIO4, com resistor em série de 1 kΩ em cada ligação como ponto de partida |
| LCD RS | P11 / S2 do PCF e uma das 16 redes passivas, escolhida após o mapa |
| LCD CS ativo em LOW | GND, com barramento SPI dedicado à tela |
| LCD RSE | GPIO12, com pull-down de 10 kΩ como ponto de partida. Retirar o pull-up de RSE da montagem anterior |
| LCD SCL / SI | GPIO15 / GPIO2 |
| Bateria | GPIO33, conforme divisor existente |

GPIO12 deve ser medido em LOW no reset na configuração de flash de 3,3 V.
Verificar eventuais pulls do módulo e ausência do microSD. O firmware mantém
RSE em LOW durante a inicialização dos outros sinais e o libera após
estabilizar a alimentação e colocar SCL em LOW. CS fixo em LOW exige clock
dedicado e transferências inteiras em múltiplos de oito pulsos. Se perder o
alinhamento, aplicar reset físico do LCD pelo GPIO12 e reinicializar.
[Boot ESP32](https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html)
e [ST7565R](https://support.newhavendisplay.com/hc/en-us/article_attachments/26189026785559).

Na varredura do teclado, manter SCL da tela em LOW. Durante a atualização do
LCD, suspender a varredura, liberar os outros sinais do teclado e controlar
somente P11 como RS. O compartilhamento pressupõe que as chaves dessa rede
fechem com outras vias liberadas e não com um terra fixo. Separar botões de
liga/desliga ou redes com referências fixas no mapa. O PCF escreve 1 para
liberar uma via. Nos GPIOs nativos, usar entrada com pull-up ou saída em LOW,
sem forçar HIGH durante a varredura. Validar níveis com o teclado de carbono.
[Datasheet PCF8575](https://www.ti.com/lit/ds/symlink/pcf8575.pdf).

GPIO4 requer isolar o acionamento do LED branco. GPIO1 e GPIO3 passam a ser
do teclado e exigem o conversor UART desconectado durante o uso. GPIO1 pode
transmitir na partida. Preparar a entrada de OTA sem serial antes de trocar
as ligações da UART. Validar partidas a frio, reset, teclas mantidas durante
o desenho da tela e acionamento do OTA. Com até 18 redes independentes, é possível
reservar P11 para RS e eliminar o compartilhamento nessa variante de LCD.

## FTDI e atualização

A serigrafia da foto identifica FT4232H Mini Module Rev. 1.1. Seu USB Mini-B
precisa de um host e software apropriado. Um computador pode usar D2XX e
bit-bang para comandar os pinos do FTDI. A montagem escolhida usa os PCFs para
o ESP32 clássico ler o teclado de forma autônoma. [Datasheet do módulo FTDI](https://ftdichip.com/wp-content/uploads/2020/08/DS_FT4232H_Mini_Module.pdf).

O firmware integrado atual tem OTA HTTP testado, ativado via comando `OTA`
na UART. Para cumprir o fluxo só por Wi-Fi, falta definir e testar um acionador
sem serial, por exemplo uma entrada de menu quando o teclado estiver mapeado.
Também falta a interface para capturar as teclas pelo Wi-Fi. Nesta etapa, o
usuário pediu o esquema. O estado é de ligação proposta e firmware pendente.
