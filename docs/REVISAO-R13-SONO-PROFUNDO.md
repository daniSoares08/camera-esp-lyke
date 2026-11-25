# Sono por SHIFT + AC; despertar por MODE

25/11/2026. **Preparados STM R13 + ESP Lyke R11, ainda não instalados.**
O usuário pediu sono profundo e apenas MODE como despertar. A restrição de
carga continua válida: não gravar enquanto a bateria estiver crítica.
Instalados continuam STM R12 + ESP R9a. Esta revisão inclui a economia e
os símbolos da preparação ESP R10, além do desligamento coordenado.

## Comportamento

- SHIFT seguido de AC funciona na tela de cálculo, resposta e demais menus.
- Durante captura/envio em andamento, pede aguardar; não interrompe uma operação.
- O STM confirma a capacidade e o sono do ESP antes de entrar em STOP1.
- O LCD recebe display-off e desliga seu circuito interno de alimentação.
- O ESP desliga rádio/câmera e entra em **deep-sleep**, sem CPU, polling ou timer.
- O STM entra em **STOP1**, sem SysTick, ADC, UART ou varredura periódica.
  Fica habilitada só a interrupção da trilha de MODE. STOP1 preserva RAM e
  GPIO; é a escolha para acordar pela matriz existente, sem acrescentar fio WKUP.
- MODE acorda o STM. Um pulso de 50 ms na UART existente acorda o ESP;
  o toque serve para despertar, sem abrir o menu automaticamente.
- Outras teclas não são fontes de despertar. Um toque instável em MODE volta
  a STOP sem acordar o ESP; o sistema confirma 35 ms antes de retomar.
- Não há Wi-Fi, OTA, câmera, relógio de aplicativo ou medição de bateria em sono.
  Após MODE, o ESP inicia normalmente, abre o portal no boot e tenta a última rede.

**Fotos não enviadas na fila temporária se perdem**, pois a PSRAM do ESP
não é preservada em deep-sleep. Resposta já salva, desenhos persistidos,
redes, configurações, mapa de teclas e arquivo privado da nuvem permanecem.
O STM mantém RAM no STOP1; SHIFT+AC limpa a entrada como antes, sem apagar
memórias/estatística. Não há gravação extra de flash no desligamento da tela
de cálculo; ajustes abertos continuam sendo salvos ao sair, como antes.

## Mesmos fios e mesma janela do LCD

| Função | Recurso existente |
|---|---|
| MODE | PB12/T02 ↔ PB9/T16, conforme mapa extraído da unidade |
| STM dormindo | PB12 como entrada com pull-up, PB9 como saída baixa; demais trilhas analógicas sem pulls |
| Despertar STM | EXTI12 por borda de descida, vetor EXTI15_10 |
| Acordar ESP | PA2 → GPIO13, a própria linha STM TX/ESP RX |
| ESP dormindo | GPIO13 RTC/ext0 em nível baixo; TX do STM permanece alto até MODE |
| Câmera dormindo | GPIO32/PWDN mantido alto por hold RTC |

O código usa o par MODE gravado no mapa e escolhe pino/EXTI correspondentes;
a tabela descreve o par atual. GPIO13 do ESP32 clássico é RTC_GPIO14.
Só GPIO32 fica em hold: **GPIO0 não é mantido baixo**, pois é strap de boot.
Ao despertar o ESP libera o mux RTC antes de usar UART/câmera.
Pinagens, loader V2, partições e janela 126×48 do leitor não mudaram.

## Confirmação, falha e recuperação

Protocolo novo `CAPS S1`, separado de `CAPS G1` para preservar gráficos antigos:

```text
STM -> ESP: SLEEP? n
ESP -> STM: SLEEP READY n
STM -> ESP: SLEEP GO n
ESP -> STM: SLEEPING n   (UART esvaziada antes do sono)
STM: aguarda assentamento, suspende periféricos e entra em STOP1
MODE: STM acorda e dá pulso baixo em PA2 por 50 ms
ESP: wake ext0 em GPIO13, boot normal e LYKE-HELLO
```

Sem GO válido e recente o ESP não dorme. Atrasos/nonce errado/duplicatas
não confirmam desligamento. Timeout da consulta não manda GO posteriormente;
timeout após GO usa pulso de recuperação e mantém a interface acordada.
Firmware do ESP antigo gera pedido de atualização, em vez de simular sono.
OTA em recepção/verificação não permite sono. No despertar, o handshake
normal restaura versão/mapeamento; não se usa uma nova ligação de reset.

## Validação

As duas compilações aprovadas. Loader recompilado coincide byte a byte com
a base V2 da R12. App STM 64.688/65.536 bytes, CRC32 `b8d4541d`;
pacote comprimido 47.273/49.152 bytes. Manifestos locais contêm SHA-256.

Testes nativos executam a interface/protocolo e a rotina de STOP com registros
simulados: só EXTI MODE, SysTick/UART suspensos, trilhas em baixo consumo,
bounce, MODE antecipado, restauração dos pinos/periféricos e pulso único ao ESP.
Também cobrem confirmação em duas etapas, mensagens atrasadas/duplicadas,
timeout e overflow; regressões de matemática, bateria, leitor, transporte,
decoder e interrupções simuladas do loader passam. Relatório:
`.build/lyke_stm32/native-tests.json`, alvo R13.

**Não houve gravação nem ensaio físico deste ciclo.** Ainda validar: sono e
MODE reais, retenção da resposta/configurações, retorno do rádio/câmera,
tensão/consumo/temperatura. Não inferir consumo da placa pelo número de µA
do chip: resistor 330 Ω, boost, reguladores e outros componentes permanecem
alimentados. O usuário pretende remover o resistor quando chegar o novo módulo;
nenhuma remoção ou troca física foi realizada.

## Aplicação depois da carga

Pacote privado `output/firmware/lyke-r13-esp-r11-preparado.zip`.
Reidentificar ESP32 clássico de 4 MiB/revisões/49 teclas antes de gravar.
Instalar **ESP R11 primeiro**, conferir boot/digest/MAP49; instalar STM R13
com `lyke-stm32-ota.bin` pelo mesmo portal e UART. Loader não precisa de USB.
Em seguida testar SHIFT+AC → MODE, rejeição das outras teclas e repetição
do ciclo. Confirmar última resposta e redes; depois medir corrente real.
O pacote DFU serve apenas para recuperação/inicialização, não para o portal.

Fontes primárias:
[Espressif: deep-sleep e ext0](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32/api-reference/system/sleep_modes.html),
[Espressif: GPIO/RTC e hold](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32/api-reference/peripherals/gpio.html),
[ST: exemplo STOP1 no G431](https://github.com/STMicroelectronics/STM32CubeG4/blob/master/Projects/NUCLEO-G431RB/Examples/PWR/PWR_STOP1/Src/main.c).
