# Firmware Lyke 101078 · STM32G431 e ESP32-CAM clássica

**Preparação de 24/11: STM R12 + ESP R9**, ainda não instalados. Leia
[desenhos roláveis e Wi-Fi](../../docs/REVISAO-R12-DESENHOS-E-WIFI.md).
Os fontes atuais compilam com `sh firmware/lyke_stm32/build.sh lyke-r12`.
O estado R11/R8 abaixo continua sendo o conjunto fisicamente instalado.

**Estado final: STM32 R11 + ESP Lyke R8**, instalados por Wi-Fi, com 49
teclas e indicador de bateria confirmados pelo usuário. Divisor corrigido
para dois 470 kilohms. Detalhes em [Bateria e R11](BATERIA-R11.md).
Manual final do comprador: [PDF](../../docs/manual/Manual-Lyke.pdf) e
[Markdown](../../docs/manual/Manual-Lyke.md). Para compilar a revisão atual,
use `sh firmware/lyke_stm32/build.sh lyke-r11`; a base V2 é a mesma abaixo.
O restante desta página preserva a instalação e os históricos anteriores.

## Estado atual e instruções da R10 — 23/11/2026

**Instalados e testados: STM32 R10a + ESP Lyke R8.** Base R10 gravada
por USB com backup/releitura e mapa/preferências preservados. Atualização
R10→R10a pelo Wi-Fi/UART existente concluída em 12,44 s. Usuário confirmou
menu, teclas e raiz/frações inteiras na janela disponível do LCD.
As instruções atuais de compilação, instalação inicial pelo USB, atualização
pelo Wi-Fi, novo mapa de memória e símbolos estão em
[Pronto para testar R10](PRONTO-PARA-TESTAR-R10.md).

Sem fios adicionais: PA2 → GPIO13, PA3 ← GPIO14 e GND comum existentes.
Instalar o novo carregador V2 junto da aplicação uma vez pelo USB; não enviar
R10/R10a ao carregador antigo. Pacote novo `LYKEG432`, app 64 KiB e staging 48 KiB.
O mapa/preferências mantêm os endereços originais.

**O conteúdo abaixo é histórico da edição R8/R5.** Seus comandos de versões
antigas, leitor de quatro linhas, mapa de 56 KiB e ligação provisória de
migração não são instruções para instalar o conjunto R10/R8.

## Histórico da edição R8/R5

A Lyke usa o LCD GMG12864-06D Ver.2.2 no SPI1 do STM32,
o teclado original de 19 trilhas com T19 em PC6, o ADC no PA0 e a UART
115200 8N1 em PA2/PA3 com o ESP32-CAM em GPIO13/GPIO14. A GPIO33 do
ESP fica como entrada. O esquema histórico está em
[TECLADO-19](../../docs/conhecimento/TECLADO-19.md).

Estado em 23/11/2026: STM32 Lyke R8 permanece instalado; ESP Lyke R5
instalada por OTA em app1, com revisão/hash conferidos. UART com 49 teclas,
captura JPEG, envio à API, arquivo privado e resposta passaram no aparelho.
A2 → GPIO13, A3 ← GPIO14 e GND comum em operação. A atualização física
do STM pelo portal, três fotos numa consulta e bateria aferida seguem pendentes.
O mapa concluído está exportado em
[teclado-lyke.csv](../../docs/conhecimento/teclado-lyke.csv).

A revisão STM32 R8 está instalada e verificada por leitura. Ela remove
o diagnóstico temporário no CLK. Em MODE → Atualizar firmware, solicita
o portal pela UART permanente A2/A3, mostra rede/endereço após confirmação
do ESP e permite reenviar com `=`. A versão instalada do ESP agora é a R5.

Na R2, apenas as duas linhas superiores do LCD são reservadas para
compensar a moldura. As seis linhas restantes usam as 21 colunas do
LCD. O leitor de respostas usa quatro linhas por página.

## Compilação

- No WSL com GCC ARM, Newlib Nano e Python 3, executar
  `sh firmware/lyke_stm32/build.sh lyke-r8`. Os dois binários do STM32
  e o manifesto de integridade saem em `.build/lyke_stm32/lyke-r8/`.
- No Windows, executar
  `\.venv\Scripts\python.exe firmware/esp32cam_integrada/build.py --profile lyke --revision lyke-r5`.
  O perfil usa Arduino ESP32 3.3.6 e ArduinoJson 7.4.2. A aplicação e o
  manifesto saem em `firmware/esp32cam_integrada/bin/lyke-r5/`.
- O mapeador R4 e os binários do perfil de bancada continuam disponíveis no
  histórico. Os binários da Lyke são para o STM32G431CBU6 e o ESP32 clássico
  AI Thinker com 4 MiB de flash e 4 MiB de PSRAM.

## Primeira instalação

1. Identificar o STM em DFU no USB de dados, ID `0483:DF11`, interface
   interna com 128 KiB. Para entrar uma vez pelo carregador de fábrica,
   segurar BOOT0, apertar e soltar NRST, então soltar BOOT0. Gravar
   `lyke-stm32-dfu.bin` a partir de `0x08000000` e conferir por leitura.
   Reiniciar o STM para abrir o mapeador. As imagens contêm o endereço e
   o hash no manifesto de compilação.
2. Instalar no ESP somente `esp32cam_integrada.ino.bin` pela OTA do perfil
   anterior. Na R4 de bancada, abrir o portal pelo UART0 (comando `OTA`
   a 460800) quando ele estiver fechado. A R6 do STM32 também envia esse
   comando. Com as duas placas desligadas, ligar temporariamente o
   pino CLK (PA14) do conector SWD do STM32 ao U0R (GPIO3) do ESP, com
   GND em comum. Manter a UART normal em A2/A3. No STM32, abrir
   **MODE → Atualizar firmware → =**; na tela que oferece abrir o ESP
   antigo, pressionar **=** novamente. Verificar o AP legado no Wi-Fi.
   Após o reinício do ESP e a confirmação de que o novo portal está
   disponível, desligar a montagem e retirar o jumper. O novo SSID
   `Calc-Lyke` responde em `http://192.168.8.1` com a senha local fixa já
   usada na bancada. O utilitário `update_once.py --keep-portal` conserva o
   portal aberto para conferir o novo conjunto.
3. Apertar e soltar as 49 teclas na sequência indicada. A R3 pula ON no
   aparelho do usuário, que usa chave física da bateria. O progresso fica
   na flash do STM e retoma após reinício. Os quatro lados do REPLAY são
   quatro entradas. O progresso também aparece no portal, se a UART está
   comunicando. Confirmar se o texto cabe de frente no recorte do gabinete.
   Na última etapa, apertar o botão físico `=`. Se ele tiver sido
   associado a uma posição anterior, a R5 pede a tecla daquela posição
   e salva as duas associações em um só registro. Revisões anteriores
   à R5 não leem esse registro de correção.
4. Fazer primeiro os testes locais de cálculo, UART, Wi-Fi e galeria.
   Com a bateria solta, o menu de tensão pode indicar ausência de leitura.
   Medir a tensão com multímetro ao reconectar para aferir o divisor de
   alta impedância. A amostragem usa PA0/ADC1_IN1, média e VREFINT calibrada.

## Uso depois do mapeamento

- MODE abre o menu, REPLAY navega, `=` seleciona e AC volta. O STM mantém
  cálculo algébrico e RPN, histórico, DEL por função, π, trigonometria,
  ângulos DEG/RAD/GRA, formatos NORM/FIX/SCI/ENG, frações, DMS, coordenadas,
  variáveis, estatística básica, constantes e gravação de um programa.
- SHIFT + seta direita tira foto. A próxima das três caixas pisca enquanto captura.
  No menu é possível abrir a galeria, remover a última captura, limpar a
  fila, enviar de uma a três fotos em uma consulta e paginar a resposta.
  As imagens permanecem na memória em caso de erro no envio. A câmera
  encerra o driver e vai para PWDN ao terminar a captura.
- A fila reserva no máximo 1,5 MiB de PSRAM e 512 KiB por JPEG. Se necessário,
  o ESP tenta compressão maior e depois resolução UXGA. A fila só aceita
  gravação quando sobra mais de 512 KiB de PSRAM. O envio HTTPS também
  confere memória interna livre e bloco contíguo antes de começar.
- Wi-Fi permite buscar redes, selecionar uma rede salva ou cadastrar a
  senha no celular. No portal, é possível salvar a chave da API e o modo
  econômico, ajustar orientação e foco da câmera e abrir a resposta. Ao
  desligar o Wi-Fi pelo menu o rádio do ESP para. O ícone no visor só
  aparece quando há conexão a uma rede externa.

## API e arquivo de fotos

A R5 usa GPT-6.1 Terra pelo serviço Supabase, com a chave OpenAI no servidor.
As fotos enviadas são arquivadas privadamente, junto da resposta. O ESP
guarda apenas a credencial própria. Detalhes e testes em
[API e arquivo](../../docs/API-E-ARQUIVO.md). Crie `.private/ap-password.txt`
com a senha local do portal antes de compilar um checkout sem credenciais.

## Atualizações futuras

- ESP Lyke R4 instalada e verificada por UART em 22/11/2026:
  `firmware/esp32cam_integrada/bin/lyke-r4/manifest.json`.
  O portal inicia ligado, sem prazo, com nova tentativa a cada 3 segundos
  caso o AP falhe. **MODE → Desligar Wi-Fi** vale só até reiniciar o ESP.
  Abrir configuração/atualização ou buscar/conectar rede reativa o portal.
  OFF é aceito no modo de atualização ocioso; uma transferência do ESP
  em andamento continua protegida. OFF pelo comando de manutenção e retorno
  do portal após reset passaram com o ESP isolado no adaptador. O teste
  pelo menu físico do STM e sem adaptador continua pendente.
- No portal, selecionar `lyke-stm32-ota.bin` em **Atualizar STM32**. O ESP
  valida o pacote e pede reinício ao STM pela UART. O carregador escreve
  em staging, confere tamanho, vetor e CRC e só então copia para o slot
  de aplicação. A área do teclado e as preferências ficam fora dos slots.
  Se a cópia ao slot ativo for interrompida, o carregador pode repeti-la
  a partir de staging no próximo boot. A rotina pode ser repetida sem
  pressionar BOOT0 ou NRST em operação normal.
- O ESP recebe seu próprio `esp32cam_integrada.ino.bin` pelo slot OTA
  inativo. O portal permanece disponível enquanto o Wi-Fi estiver ligado.
- Endereços STM32 em flash: carregador `0x08000000`, aplicação
  `0x08002000` (56 KiB), staging `0x08010000` (56 KiB), commit
  `0x0801e000`, teclado `0x0801e800`, preferências e programa
  `0x0801f000` e `0x0801f800`.

A instalação física dos dois firmwares e a largura do LCD foram
confirmadas. A OTA do STM32 via ESP, o teste com alimentação fria e
os testes da câmera e da API com o conjunto integrado continuam pendentes.
