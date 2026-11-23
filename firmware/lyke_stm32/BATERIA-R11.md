# Lyke R11 - indicador de bateria

Instalado em 23/11/2026 por Wi-Fi do ESP Lyke R8 e UART existente,
14,16 segundos, HTTP 200 e reinício confirmado como `lyke-r11`, 49 teclas
e `keyboard_ready=true`. Nenhum pino foi alterado. O usuário confirmou
que o ícone aparece e MODE > BATERIA continua indicando tensão normal.

## Divisor corrigido pelo proprietário

O usuário mediu 3,84 V com multímetro e identificou um resistor de
amarelo-roxo-marrom (470 ohms) no lugar de amarelo-roxo-amarelo (470 kilohms).
Trocou para dois de 470 kilohms e informou que a leitura normalizou.
A posição do resistor incorreto não foi medida pelo agente; não inferir
uma calibração exata a partir da fotografia anterior à troca final.

## Indicador e limites

O desenho ocupa x=94..105/y=0..7 do cabeçalho lógico, imediatamente antes
dos três quadrados de fotos. Continua dentro dos 126×48 pixels úteis;
as duas linhas escondidas pela moldura não foram usadas. O leitor da IA
continua sem ícones, com seis linhas somente de conteúdo.

| Indicação | Comportamento |
|---|---|
| 0 a 4 barras | Faixas aproximadas pela tensão, não porcentagem exata |
| ! piscando | Leitura válida abaixo de 3,45 V; pisca a cada 500 ms |
| ? | Sem leitura, saturação, faixa inválida ou tensão incompatível |

Transições nominais: 3,45 / 3,65 / 3,85 / 4,05 V, com histerese de 40 mV
nas barras. Não há sinal elétrico de carregamento; o ícone não o indica.
Wi-Fi e funcionamento normal não são desligados automaticamente por ele.

A medição inicia ao ligar e se repete 30 s após a última conclusão.
O estado do ADC avança em pequenas etapas no laço principal, permitindo
continuar teclado/UART durante assentamento, calibração e amostragem.
VREFINT/PA0 usam média de 16 amostras após descartar a primeira, com
amostragem longa; ADC/regulador/referência são desligados ao terminar.
Falhas têm timeout e invalidam a indicação. Entrar no loader cancela uma
medição ativa. O comando explícito = no menu ainda mede antes de retornar.

## Verificação

Compilação ARM e testes nativos aprovados: níveis/histerese/erros, desenho
e limites da janela, fotos intactas, leitor sem cabeçalho, driver ADC com
hardware simulado, temporização de 30 s, timeout, retorno de relógio de
32 bits e cancelamento para OTA. Testes do loader: 514 + 204 pontos de
interrupção simulados; não houve corte elétrico durante a OTA real.

```powershell
wsl --cd /mnt/c/Projects/CAMERA_ESP sh firmware/lyke_stm32/build.sh lyke-r11
.venv\Scripts\python.exe firmware/lyke_stm32/tests/run_native.py
.venv\Scripts\python.exe scripts/preview_battery.py
```

Manifesto: `.build/lyke_stm32/lyke-r11/manifest.json`.
Aplicação: 60.592 bytes, CRC32 `ab7062c2`, SHA-256
`e317b4ee341e6173b91d54f8ee039d8a43c2b8d39e038fdae3d11ddf98ddeb38`.
OTA: 44.134 bytes, SHA-256
`8ea1847c0dd8af559ac526e21a70718f2fbfbb685c458ca7f033bddfa6c0d54b`.
Loader V2 permanece idêntico. Relatório de instalação privado:
`.private/lyke-cloud/device-stm-r11-wifi.json`.

O manual final do comprador é edição 2.0, STM R11/ESP R8, em
`docs/manual/Manual-Lyke.pdf` e `.md`. Binários e credenciais não são publicados.
