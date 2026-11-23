# Calculadora científica com consulta por fotografia

O aparelho inicia como calculadora comum e calcula sem internet. O S3 lê as 46 teclas medidas, mantém a interface, usa Wi-Fi e fotografa com a OV5640. O ESP32 DevKit V1 recebe a imagem da interface por UART e aciona o LCD LCM12864D-V1.0 WHITE. Esta é uma aplicação científica própria; não emula a ROM, o sistema simbólico ou a linguagem RPL da HP 48.

## Ligação usada pelos firmwares

Os números são GPIOs, não posições do conector. DevKit considerado: ESP32-WROOM-32, flash de 4 MB. S3 considerado: GOOUUU S3-CAM N16R8, flash de 16 MB e PSRAM OPI de 8 MB.

| Origem S3 | Destino DevKit |
|---|---|
| TX / GPIO43 | RX2 / GPIO16 |
| GND | GND |

UART: 115200, 8N1, apenas S3 → DevKit. GPIO17/TX2 do DevKit fica sem conexão. O RX44 do S3 permanece para seu conversor USB/UART CH343. O quadro possui sincronismo, versão, sequência, tamanho e CRC16; um quadro incompleto ou corrompido é descartado. O S3 repete a imagem a cada segundo para recuperar uma conexão que começou depois do boot.

O DevKit gira toda a imagem em 180° por padrão, pois o LCD foi instalado de ponta-cabeça. A rotação inclui calculadora, menus, mensagens de conexão e diagnóstico; permanece após desligar. As setas mantêm suas funções normais na orientação de leitura.

Atualização sem abrir a carcaça: E → **Atualizar firmware** → escolha S3 ou tela. As duas placas precisam da instalação inicial por USB com as novas partições. Depois, cada uma abre sua rede temporária com senha no LCD, e recebe o aplicativo pelo navegador em `http://192.168.8.1`. Veja [ATUALIZAR-POR-WIFI.md](ATUALIZAR-POR-WIFI.md) para os arquivos corretos e o procedimento de recuperação.

Use a USB/UART do S3. Os GPIO19/20 estão no teclado; mantenha a USB OTG desconectada.

| Pino do LCD | Sinal | Ligação no DevKit/alimentação |
|---:|---|---|
| 1 | VDD | +5 V regulados |
| 2 | VSS | GND |
| 3 | VO | Contraste |
| 4 | DB0 | GPIO4 |
| 5 | DB1 | **GPIO14**, conforme troca feita pelo usuário |
| 6 | DB2 | **GPIO13**, conforme troca feita pelo usuário |
| 7 | DB3 | GPIO18 |
| 8 | DB4 | GPIO19 |
| 9 | DB5 | GPIO21 |
| 10 | DB6 | GPIO22 |
| 11 | DB7 | GPIO23 |
| 12 | CS1 | GPIO25 |
| 13 | CS2 | GPIO26 |
| 14 | RST | GPIO27 |
| 15 | RW | GND permanente; somente escrita |
| 16 | RS | GPIO32 |
| 17 | E | GPIO33 |
| 18 | VOUT/VEE | Saída negativa para contraste, não vai a GPIO/GND |
| 19 | LEDA | +5 V por resistor de 220 Ω, ou aberto se não usar backlight |
| 20 | LEDK | GND |

CS1/CS2 são ativos em LOW nessa versão. Os níveis de saída de 3,3 V atendem ao HIGH mínimo de 2 V da folha do vendedor salva em `firmware/demo_lcm12864/especificacao_vendedor.png`. Confirme a numeração na placa: esta tela tem **pino 1 positivo e pino 2 GND**.

Diagnóstico do LCD pela USB do DevKit, a 115200 baud: envie `t` para alternar entre metade esquerda, metade direita, texto com borda e tela limpa, quatro segundos por fase; `n` restaura a calculadora, `r` reinicializa o LCD e `s` informa os contadores de quadros/CRC. O modo de teste não altera a configuração do S3 e termina ao reiniciar o DevKit. A versão v2 usa tempos mais lentos no barramento e no reset. Com RW no GND, os contadores confirmam apenas a recepção dos quadros pelo DevKit; a imagem física precisa de conferência visual.

Sem potenciômetro: ponto inicial de contraste com +5 V → 15 kΩ → VO/pino 3 → 5,1 kΩ → VEE/pino 18. Se VEE for −10 V, resulta em VO de cerca de −6,2 V. A legibilidade precisa ser conferida fisicamente; variar o divisor pode ser necessário. O resistor do backlight é separado do divisor de contraste.

Com bateria Li-ion/LiPo 1S, use conversor elevador regulado em 5 V e carregador/proteção apropriados. Una GNDs. Durante gravação por USB, desconecte a alimentação externa de 5 V; as fontes não devem ser colocadas em paralelo sem um circuito de seleção adequado. Não ligue uma célula diretamente aos pinos 3V3.

LEDs: o firmware do DevKit mantém GPIO2 em LOW para apagar o LED de usuário, conforme a [documentação da placa DOIT no Zephyr](https://docs.zephyrproject.org/latest/boards/others/doit_esp32_devkit_v1/doc/index.html). No S3, tenta enviar preto ao RGB em GPIO48 uma única vez antes de iniciar o teclado, libera o periférico RMT e devolve o pino à varredura. Se GPIO48 estiver LOW mesmo com pull-up, não força saída HIGH: o comando é omitido e o estado USB informa `rgb_off_command=skipped_gpio48_low`. LEDs ligados às fontes e o LED ligado ao TX no esquema GOOUUU não possuem desligamento independente por software. O TX43 é necessário ao LCD; não é mantido desativado. Apagar todos os LEDs requer identificar e remover fisicamente os LEDs/resistores correspondentes na revisão real das placas. O esquema GOOUUU arquivado em `dados/laboratorio/GOOUUU_schematic.png` é referência e não confirma sozinho a revisão CH343 conectada. Nenhuma corrente foi medida.

## Calculadora

Modo comum por padrão: `2 + 3 × 4 ENTER` resulta em 14. Parênteses têm precedência; potência associa à direita; `-2^2` resulta em −4. São usados números reais de dupla precisão, exibidos com até 12 algarismos significativos. Não há cálculo simbólico, números complexos ou sistema de unidades automático.

| Tecla | Ação |
|---|---|
| 0–9, ponto | Digitação |
| +, −, ×, ÷, y^x | Operações |
| ENTER curto / EVAL | Avaliar a conta; ENTER duplica X em RPN com entrada vazia |
| **E** | Abrir menu |
| COS / TAN | Funções; também disponíveis pelo menu MTH |
| Shift esquerdo + COS/TAN | acos / atan |
| Shift direito + COS/TAN | cosh / tanh |
| √x / shift + √x | Raiz / quadrado |
| 1/x / shift esquerdo / shift direito | Recíproco / ln / exp |
| +/− | Sinal da entrada ou do expoente |
| EEX | Notação científica: `1 EEX 3 +/−` produz `1E-3` |
| STO | Escolher A–F para guardar um valor |
| VAR | Escolher A–F para recuperar um valor |
| CST | Constantes pi, e, c, g, NA e kB |
| MTH | SIN, inversas, hiperbólicas, log, ln, exp, fatorial e outras |
| PRG | Gravar, executar e apagar uma sequência básica de teclas aritméticas |
| NXT | Alternar os três conjuntos de teclas de função A–F |
| Setas cima/baixo | Histórico; em menus, selecionar item |
| Setas esquerda/direita | Cursor; em menus, mudar uma página |
| DEL / shift + DEL | Limpar entrada / limpar a calculadora |
| Apagar | Remover função/constante inteira ou um dígito; em RPN sem entrada, remover X |
| ON | Voltar à calculadora; nela, limpar conta e pilha, preservando memórias |
| SPC | Capturar uma foto para a consulta, até três |

Teclas ausentes aceitas no mapeamento: M/apóstrofo, S/SIN e ALPHA. SIN está em MTH e na segunda página A–F. O cadastro de texto possui um seletor completo de caracteres, incluindo M, S, apóstrofo, espaço e símbolos; não depende da tecla ALPHA física.

Páginas A–F: `( ) PI ANS MNU MTH`; E sempre abre o menu. Demais páginas: `SIN LOG LN EXP SQR DEG`; `MEM STO RCL PRG CST CFG`. Grau/radiano e modo comum/RPN ficam nas configurações. O fatorial aceita inteiros de 0 a 170. Erros de domínio, divisão por zero e valores não finitos são informados.

Memórias A–F, modo, ângulo, modelo, orientação da foto e programa persistem após reiniciar. A conta em edição, pilha e histórico da sessão ficam em RAM. A gravação PRG registra até 96 teclas aritméticas; não grava navegação de menus nem executa código vindo da IA.

## Menu e Wi-Fi

Pressione E. Use setas ou A/B para selecionar, ENTER para confirmar e F/ON para voltar. A primeira opção é Wi-Fi; a segunda é **Enviar fotos para IA**. Há leitor da última resposta, configurações, configuração pelo celular e ajuda.

Os dois perfis informados pelo usuário estão pré-cadastrados: **DANIEL-HUAWEI** e **IPHONEPROMAX**. A migração adiciona/atualiza o novo nome do iPhone nas instalações existentes sem apagar os outros perfis. As senhas ficam em `secrets.local.h`, ignorado pelo Git, e são gravadas no NVS na primeira inicialização. Elas não são mostradas no LCD nem retornadas pelo configurador. O arquivo e os binários são privados: contêm esses perfis.

O Wi-Fi inicia desligado. Escolha uma rede salva no menu quando quiser consultar. Há busca de redes próximas e cadastro de até oito redes. Cadastrar novamente o mesmo SSID atualiza a senha. A conexão tem tempo limite e mensagem de erro. No iPhone, ative **Maximizar Compatibilidade** se o hotspot não aparecer; o ESP utiliza Wi-Fi de 2,4 GHz.

No editor: A/B/C/D escolhem minúsculas/maiúsculas/números/símbolos; esquerda/direita escolhem o caractere; ENTER insere; DEL/BACKSPACE remove; F salva. Dígitos físicos podem ser digitados diretamente. SSID: até 32 bytes. Senha WPA: 8–63 caracteres, ou vazia para rede aberta.

## Configurar a API com privacidade

Opção USB: abra `Configurar calculadora.bat` na raiz. Selecione a porta da USB/UART do S3, conecte, cole a chave no campo protegido e salve. O configurador mostra os quadros realmente enviados pelo S3 e o estado do aparelho. Ele não salva a chave no computador. Feche o antigo mapeador e outros monitores seriais antes de conectar.

Opção celular: menu → **Configurar pelo celular**. O LCD mostra uma rede temporária `CalcSetup-…`, senha fixa CHANGE_ME_ESP_AP e endereço `192.168.8.1`. Conecte-se a essa rede e abra o endereço. O formulário permite configurar API e adicionar Wi-Fi. A rede de configuração fecha após dez minutos. O formulário só atende pela interface da rede temporária, tem token de requisição e não retorna a chave.

Este firmware usa **uma chave de API própria de projeto**, enviada diretamente a `https://api.openai.com/v1/responses`. Não extrai login, cookies ou tokens do Codex/ChatGPT. As consultas são feitas pelo projeto da API e não são inseridas automaticamente nesta conversa. Uma integração de login com plano ChatGPT exigiria outro fluxo de autorização; ela não foi implementada neste firmware.

Crie/configure a chave em https://platform.openai.com/api-keys e consulte saldo/recarga em https://platform.openai.com/settings/organization/billing/overview . Para começar os testes, um orçamento de **US$5** é suficiente como saldo inicial sugerido; confirme no painel o valor mínimo e os limites da sua conta. Nenhuma compra foi realizada pelo agente.

Modelos configuráveis, preços de referência em 27/10/2026 por milhão de tokens:

| Modo | Modelo | Entrada | Saída |
|---|---|---:|---:|
| Preciso, padrão | gpt-6.1-sol | US$2,00 | US$10,00 |
| Econômico | gpt-6-luna | US$0,10 | US$0,50 |

Uma consulta com 2.000 tokens de entrada e 1.000 de saída custaria aproximadamente US$0,014 no preciso ou US$0,0007 no econômico. A foto e o raciocínio entram no consumo; o total varia por imagem e resposta. O aparelho mostra os tokens retornados pela API e uma estimativa. O valor faturado deve ser consultado no painel. Limite de saída por pedido: 4.096 tokens; nenhuma ferramenta de pesquisa ou geração de imagem é ativada.

TLS valida certificado/hostname com o bundle de CAs do ESP-IDF e exige relógio sincronizado. Não há `setInsecure`. As chamadas usam `store:false`. Não existe repetição automática depois do envio, evitando consultas duplicadas involuntárias. Chave inválida, falta de saldo, limites de projeto, timeout, recusa e resposta incompleta têm mensagens próprias. Os dados de autenticação em NVS não possuem criptografia de flash configurada neste protótipo; quem tem acesso físico ao aparelho pode extraí-los.

## Fotografar e ler

Posicione o exercício com iluminação uniforme, texto grande e foco adequado. Capture de uma a três fotos com **SPC**, aguardando cada quadradinho preencher. Não é obrigatório usar as três posições. Depois conecte Wi-Fi, configure a chave se necessário e use **E → Enviar fotos para IA**. As fotos seguem juntas e em ordem no mesmo pedido. A calculadora continua responsiva enquanto a tarefa de rede trabalha; ON permite voltar a calcular. A consulta é enviada somente após essa seleção explícita.

A OV5640 é inicializada sob demanda, fotografa em JPEG na resolução QSXGA suportada pelo driver, com um framebuffer, e usa a lente de foco fixo deste módulo, sem carregar firmware de autofoco nem aguardar confirmação de foco. Assim que copia o JPEG para a PSRAM, o firmware solicita software power-down no registrador 0x3008, confirma o bit quando possível, encerra o driver e mantém XCLK parado. Isso acontece **antes de codificar/enviar a foto por HTTPS**, inclusive com limpeza em caminhos de erro. Ela será reinicializada na próxima fotografia. A configuração USB permite verificar `camera_interface_active` e `camera_standby_confirmed`.

Isso é standby e desligamento da interface, **não corte físico de toda a alimentação**. O esquema publicado da GOOUUU mantém os reguladores da câmera alimentados e não fornece um GPIO PWDN utilizável na configuração atual; a correspondência exata do esquema com cada revisão não foi comprovada. Consumo zero do módulo exigiria uma chave de alimentação externa e projeto que evite realimentação pelos sinais. Não foi feita medição de corrente nesta etapa.

A resposta pede transcrição, passos principais e resultado em texto compatível com o LCD: `^`, `*`, `/`, `sqrt()`, `pi` e notação E, sem LaTeX. Setas cima/baixo rolam por linha; esquerda/direita, por página. A–D também oferecem rolagem/paginação. A última resposta bem-sucedida fica salva na flash. Erros não apagam essa resposta. As respostas da IA são exibidas como texto, nunca executadas como comandos ou programas. Confira a transcrição e o resultado; visão e cálculo do modelo podem errar.

## Compilar, testar e gravar

Requisitos presentes neste computador: Arduino CLI, core ESP32 3.3.6, U8g2 2.36.19, ArduinoJson 7.4.2, Python do projeto, GCC e Node.js para testes no PC.

```powershell
& .\firmware\calculadora\Testar.ps1
& .\firmware\calculadora\Compilar.ps1
& .\firmware\calculadora\Gravar.ps1 -Alvo S3 -Porta COM12
& .\firmware\calculadora\Gravar.ps1 -Alvo Devkit -Porta COMx
```

Use as portas reais. O S3 e o ESP32 clássico têm bootloaders/offsets diferentes; o script escolhe pelo alvo e confere hashes. O perfil do DevKit usa flash de 4 MB; confirme o módulo antes de gravar. A gravação substitui o aplicativo e a tabela de partições, mantendo a área NVS. Os dois slots OTA têm 3 MiB cada no S3 e 1,5 MiB cada no DevKit; o envio por Wi-Fi usa somente o arquivo de aplicativo `.ino.bin`.

Os binários ficam em `firmware/calculadora/bin/s3_calculadora` e `firmware/calculadora/bin/devkit_display`. Cada pasta tem manifesto de hashes. `secrets.example.h` é o modelo público; `secrets.local.h` deve permanecer privado. O firmware da calculadora não usa os fontes antigos de autofoco do projeto.

Verificação em 03/11: três JPEGs distintos 2560×1920 conferidos pelo Wi-Fi, sensor em standby e interface inativa após capturas, busca real com nove redes incluindo IPHONEPROMAX. A conexão ao hotspot não foi confirmada. O DevKit aceitou o envio OTA, mas falta conferir seu slot após reiniciar. A revisão final do S3 com diagnóstico de conexão está pronta para envio quando a bateria carregar. Consulta real com múltiplas imagens e medição de corrente continuam pendentes; nenhum crédito de API foi usado nesses testes. As prévias em `outputs/calculadora/previas` vêm do mesmo renderizador C++ usado pelo S3; não são fotografias do painel físico.

## Referências

- https://developers.openai.com/api/docs/guides/images-vision?api-mode=responses
- https://developers.openai.com/api/docs/models/gpt-6.1-sol
- https://developers.openai.com/api/docs/models/gpt-6-luna
- https://developers.openai.com/api/docs/pricing
- https://developers.openai.com/api/docs/guides/error-codes
- https://github.com/espressif/esp32-camera/blob/master/sensors/private_include/ov5640_regs.h
- https://github.com/zhuhai-esp/ESP32-S3-Goouuu-Cam

O pequeno font bitmap utilizado no renderizador vem da Adafruit GFX; sua licença está em `common/FONT-LICENSE.txt`.

Visualização do JPEG original e captura sem API: consulte [VER-FOTO-NO-WIFI.md](VER-FOTO-NO-WIFI.md). Endereço atual das redes locais: 192.168.8.1; senha solicitada pelo usuário: CHANGE_ME_ESP_AP.

Revisão de 03/11/2026: consulte [ATALHOS-E-FOTOS.md](ATALHOS-E-FOTOS.md). SPC captura de uma a três fotos; E → Enviar fotos para IA transmite o conjunto. E fica reservado ao menu em todas as páginas de atalhos.
