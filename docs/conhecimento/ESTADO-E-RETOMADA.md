# Estado e retomada — 24/11/2026

## Próxima retomada

### Preparados STM R12 + ESP Lyke R9; usuário ligará depois

Pedido de 24/11/2026: conferir rede Visitantes salva no Windows; ocultar
sinal Wi-Fi normal, piscar três vezes no sucesso e alternar X/sinal na
falha; preparar desenhos roláveis entre os passos da resposta. **Não houve
gravação física nesta etapa. Instalados continuam STM R11 + ESP R8.**

- Perfis `Visitantes` e `VISITANTES` verificados sem exibir/exportar senha:
  ambos abertos, sem chave. Usuário confirmou internet direta, sem login.
  Compatíveis com o caminho de rede aberta do ESP; frequência não consta
  no perfil. Exige cobertura 2,4 GHz no campus, ainda não testada.
- Compilados STM `lyke-r12` e ESP `lyke-r9`. Loader V2/mapa de flash
  inalterados. App STM 62.568 bytes/CRC `37e35397`, OTA 45.530 bytes.
  ESP 1.362.784 bytes, dentro do slot. Hashes nos manifestos desses alvos.
- Estado Wi-Fi comunicado por LINK: três pulsos de 300 ms, depois oculto;
  falha alterna sinal/X a cada 500 ms até reconectar/desligar. Não aparece
  no leitor da resposta. Sem aviso permanente de sucesso no rodapé.
- Leitor novo READG recebe janelas monocromáticas 126×48, 768 bytes,
  numeradas e verificadas por checksum/máscara de 48 linhas. Texto e
  desenhos convivem no documento do ESP. Setas verticais rolam 16 pixels;
  horizontais movem 24 em figura larga ou 48 verticais em região estreita.
- API v3 **publicada** no serviço existente. Só solicita documento JSON
  quando o cliente manda `display=lyke-graphics-v1`; clientes antigos
  continuam em texto. Schema estrito/comandos limitados e sem código/URLs.
  JSON bruto no `answer_text`, texto compatível em `answer_lcd`; sem migração.
  Dezesseis testes locais aprovados. GET confirma versão 3, POST sem token
  retorna 401. Nenhuma chamada paga/consulta de fotos nova nesta preparação.
- Testes nativos: Wi-Fi, frames, interface, circuito/gráfico/ângulo,
  rolagem, compatibilidade e regressões aprovados; loader 514+209 pontos
  simulados. Relatório `.build/lyke_stm32/native-tests.json`. Os exemplos
  de desenho são determinísticos; ainda não são uma resposta real do modelo.
- **Próximo:** após usuário ligar, identificar via portal, instalar aplicação
  ESP R9 e STM R12 por Wi-Fi; conferir revisões/teclas/bateria/animação;
  validar uma questão com figura no aparelho. Não afirmar instalação feita.
- Guia e prévia em `docs/REVISAO-R12-DESENHOS-E-WIFI.md` e
  `output/previews/lyke-r12-desenhos.png`. Manual 2.0 permanece edição R11/R8,
  correspondente ao hardware instalado; revisão nova está descrita no guia.

### Histórico de 23/11: encerramento R11, bateria e manual


Em 23/11/2026, o usuário informou 3,84 V no multímetro e corrigiu um
resistor amarelo-roxo-marrom (470 ohms) para dois amarelo-roxo-amarelo
(470 kilohms), normalizando a leitura. Não confundir a foto anterior,
com 4,04 V no LCD, com uma calibração exata após a correção.

- **Instalados STM R11 + ESP Lyke R8 (app0).** R11 enviada pelo portal,
  Wi-Fi/UART existente, 14,16 s, HTTP 200, reinício e revisão confirmados.
  Mapa de 49 teclas/`keyboard_ready=true` preservados. Loader V2 inalterado.
  Manifesto `.build/lyke_stm32/lyke-r11/manifest.json`; registro privado
  `.private/lyke-cloud/device-stm-r11-wifi.json`.
- Ícone de bateria imediatamente antes das fotos, x=94..105/y=0..7 do
  cabeçalho lógico. Quatro níveis aproximados, ! piscando quando baixa,
  ? em leitura inválida. Janela 126×48 preservada e leitor IA sem ícones.
  Medição automática cooperativa, a cada 30 s, sem espera no teclado/UART.
  **Usuário confirmou:** ícone aparece e tensão no menu continua normal.
  Detalhes/testes em `firmware/lyke_stm32/BATERIA-R11.md`.
- App 60.592 bytes/CRC32 `ab7062c2`, SHA-256
  `e317b4ee341e6173b91d54f8ee039d8a43c2b8d39e038fdae3d11ddf98ddeb38`.
  OTA 44.134 bytes, SHA-256
  `8ea1847c0dd8af559ac526e21a70718f2fbfbb685c458ca7f033bddfa6c0d54b`.
  Testes nativos aprovados: ADC cooperativo/timeouts/período/cancelamento,
  barras/histerese/avisos/limites do LCD e regressões; loader 514+204
  interrupções simuladas. Nenhuma chamada paga nova à API.
- Usuário tornou o repositório **público**, confirmado pelo conector GitHub,
  e pediu atualização dos fontes; somente fontes/documentos sanitizados,
  sem binários de firmware, credenciais ou backups. O arquivo de fotos no
  Supabase continua privado. Organização/projeto/serviço não foram alterados.
- **Manual final edição 2.0**, 36 páginas, STM R11/ESP R8: PDF e Markdown
  com imagens em `output/pdf/`, ZIP completo; cópia de publicação em
  `docs/manual/`. Atualizados bateria, notação, gregas, seis linhas de
  resposta, OTA do STM testada e instruções OpenAI/Claude. Somente OpenAI
  integrada/testada. Inclui foto da unidade fornecida pelo usuário, com
  legenda de que é anterior ao ícone R11. Fontes oficiais reconsultadas;
  36 páginas renderizadas/inspecionadas, links de imagens e ZIP verificados.
- Usuário pediu atualizar o manual e declarou **“Finalizamos o projeto”**.
  Escopo atual concluído. Ensaios não executados (por exemplo, queda real
  de energia durante gravação e aferição de autonomia/circuito de recarga)
  são limites registrados, não resultados inventados nem trabalho em curso.

### Histórico anterior: instalação STM R10a + ESP Lyke R8


Em 23/11/2026, usuário conectou o STM e autorizou a gravação. Entrou em
DFU por BOOT0/RST. Identificado `0483:df11`, serial `209D35994333`, flash
interna `64*02Kg` (128 KiB). Backup integral coincidiu com app/loader R8
conhecidos antes de gravar. O descritor sem zeros à esquerda exigiu corrigir
somente a validação textual do utilitário USB; teste de ambos os formatos
e rejeição de alvo/endereço/tamanho diferentes aprovado.

- **Base R10 + loader V2 instalados por USB.** Releitura integral conferida:
  programa/loader idênticos ao pacote e últimos 6 KiB de mapa/preferências
  idênticos ao backup. Relatório `.private/stm32_lyke_r10_20261123_144822/result.json`.
  Usuário reiniciou e confirmou menu/teclas funcionando.
- **ESP Lyke R8 instalado por OTA em app0**, binário 1.320.496 bytes,
  SHA-256 `e7fa0fc1b9ea0b66e65a6ad16d267c852666cec5eab7054a151c707041984c22`,
  digest em execução `1863e2d7a1c57deb9259901435a5121ba097f13e09710f11ef4a6961685546a7`.
  Revisão/slot/digest conferidos; URL de serviço/acesso configurado preservados.
  Relatório `.private/lyke-cloud/device-r8-ota.json`.
- **STM Lyke R10a instalado depois pelo Wi-Fi do ESP e UART existente**,
  sem usar USB para essa transferência. HTTP 200, `STM32 atualizado e
  reiniciado`, 12,44 s. CRC app mudou de `3ab001a5` para `33e2549c`;
  loader confere CRC após escrever antes de informar sucesso. Portal passou
  de `lyke-r10` a `lyke-r10a`, mantendo 49 teclas e `keyboard_ready=true`.
  R10a tem mesmas funções, somente identificador diferente para comprovar
  troca real de aplicação, incluindo cópia do staging para o app.
- Manifesto atual `.build/lyke_stm32/lyke-r10a/manifest.json`; app 59.932 bytes,
  SHA-256 `e37c0e2069c804b1cb0a458603c39d2c2807e7ca051fe4d14f558e6c8992e192`.
  Pacote OTA 43.424 bytes, SHA-256
  `dd2783e8ffd9677e03b254c4ec8c69de55cd1b6149a8a7ff4806ae7d07994c8c`.
  Relatório `.private/lyke-cloud/device-stm-r10a-wifi.json`.
- **Usuário confirmou no aparelho:** menu e teclas; depois raiz `√(9+16)`
  e soma com tecla de fração cabem inteiras na janela original e funcionam.
  Mantidos 126×48 pixels úteis/6 linhas, sem usar as duas linhas ocultas.
  Não confundir com uso dos 128×64 completos. Cabeçalho normal ocupa parte
  dessa janela; leitor IA preparado usa as seis linhas só para resposta.
- Testes nativos repetidos para o binário R10a e utilitário ajustado, aprovados.
  Fontes, manifestos, pacote local e validação técnica atualizados.
  Instruções em `firmware/lyke_stm32/PRONTO-PARA-TESTAR-R10.md`, resumo em
  `docs/VALIDACAO-R10-2026-11-23.md`. Pacote continua na pasta
  `output/firmware/lyke-r10-pronto/` e ZIP correspondente, agora com R10a.
- **Pendentes:** arranque frio, nova consulta/leitor de seis linhas no
  conjunto atual, conferência de fotos após OTA, bateria com multímetro e
  manual PDF/MD. Últimas duas tarefas adiadas pelo usuário. Sem API paga
  nesta instalação. GitHub não atualizado nesta etapa; binários seguem privados.

### Histórico da preparação: R10 + ESP Lyke R8, antes da gravação

Em 23/11/2026, usuário pediu atualização do STM pelo Wi-Fi do ESP sem
nova solda e notação matemática melhor. **Deixar preparado; ele conectará
o STM depois para testar. Nenhum firmware foi gravado nesta etapa.**
Instalados continuam **STM32 R8 + ESP Lyke R7**. API v2 segue publicada.
R10 substitui R9 como candidata preparada; não usar as instruções antigas
de instalar só aplicação R9 na próxima sessão.

- Preparados **STM32 R10 + ESP Lyke R8**. Instruções atuais em
  `firmware/lyke_stm32/PRONTO-PARA-TESTAR-R10.md`; pacote local privado em
  `output/firmware/lyke-r10-pronto/` e `output/firmware/Lyke-R10-pronto.zip`.
- UART existente PA2→GPIO13, PA3←GPIO14 e GND comum. Sem pinos novos.
  Aplicação entra diretamente no loader; timeout sem transferência retorna
  ao app válido. Carregador anuncia `LYKE BOOT V2`; ESP recusa o antigo
  `LYKE BOOT` antes de FLASH, e recusa pacotes antigos `LYKEG431`.
- **Primeira instalação conjunta pelo USB do STM: loader V2 + app R10.**
  Não instalar `main.bin` sozinho. Novo app tem 64 KiB em `0x08002000`,
  staging compactado 48 KiB em `0x08012000`. Loader 8 KiB, meta `0x0801e000`,
  mapa `0x0801e800`, preferências `0x0801f000/0x0801f800`. Formato `LYKEG432`:
  DEFLATE por blocos independentes de até 4096 bytes, CRC codificado e plain.
- `bootstrap_usb.py` sem `--install` apenas valida, sem hardware. Quando
  conectado: reidentificar chip/DFU/128 KiB/serial, backup integral, gravar
  `lyke-stm32-dfu.bin` de 124.928 bytes e reler. O pacote limpa staging/meta
  antigos e para antes dos últimos 6 KiB de mapa/preferências, comparados
  byte a byte pelo utilitário. Não usar mass erase/option bytes.
- App compilado 59.932/65.536 bytes, pacote OTA 43.424/49.152 bytes, loader
  5.104/8.192 bytes. ESP R8 1.320.496/1.572.864 bytes. Hashes definitivos
  nos manifestos originais e no `release.json` do pacote; não inferir versão
  instalada pelos arquivos compilados. Binários privados, sem publicação.
- Frações de entrada/resultado empilhadas, raízes com barra, expoentes,
  π/Δ/θ/Ω, cursor e rolagem. Fallback linear para altura/layout incompatível.
  ALPHA,^ → Δ; ALPHA,log → θ; ALPHA,EXP → Ω. Variáveis numéricas novas,
  RCL/STO suportados; SHIFT,EXP continua π. Leitor de seis linhas e ADC
  diagnóstico da R9 incluídos. Prévias do código em `output/previews/lyke-r10-telas.png`.
- Testes nativos aprovados: código real de renderer/interface/loader/ESP,
  514 interrupções no exemplo + 203 amostras na aplicação completa, corrupção,
  limites do decoder, 250 mutações com CRC externo recalculado, falta de ACK,
  loader antigo, autorização, RAM, preservação USB e regressões de bateria/páginas.
  Relatório `.build/lyke_stm32/native-tests.json`; repetir via
  `firmware/lyke_stm32/tests/run_native.py`. Não é teste elétrico/DFU real.
- **Próximo teste físico:** instalar base STM pelo USB com backup; conferir
  teclas/LCD; atualizar só aplicação ESP R8 pelo portal atual; então testar
  OTA do STM pelo portal e arranque frio. Sem API paga nesta preparação.
  Manual/PDF e multímetro da bateria continuam para depois.

### Histórico imediatamente anterior: API v2, ESP R7 e candidata STM R9

**Ajustes de uso físico em andamento (23/11/2026, após o manual):** usuário
pediu leitor com as seis linhas dedicadas somente à resposta, sem Wi-Fi,
quadrados ou título; cabeçalho normal preservado fora do leitor. Pediu
desenvolvimento completo das contas em formato de avaliação, sem diálogo.
Solicitou deixar a revisão do manual para depois e a medição física da bateria
por último, pois a calculadora está fechada.

- **Já publicado:** serviço `lyke-solve`, versão pública `2`. Instruções
  exigem fórmula, substituição, operações intermediárias e todos os itens;
  sem limite artificial de 500 palavras e sem linhas vazias. `toLCD` remove
  linhas vazias e evita partir números quando há operador onde quebrar.
  Onze testes locais aprovados, sem mudança em autenticação, banco ou RLS.
- Consulta real do usuário com três fotos: `85bc153fbdeca15250cb0c40d70145b7`.
  A resposta tinha fórmulas, mas omitia várias substituições/desenvolvimentos;
  99 linhas, 26 vazias. Fotos e respostas originais preservadas.
- Um novo teste pago autorizado pelo ajuste reutilizou as mesmas fotos:
  `47ea7e151a984220968f84aedc828d3f`, 9.467 tokens de entrada/4.688 de saída,
  74,8 s, completo e arquivado. A resposta trouxe os cálculos intermediários
  e todos os casos, 188 linhas não vazias. Valores principais conferidos
  com cálculo independente em Python. Imagens/relatórios na pasta privada.
- **ESP instalado agora: Lyke R7 em app1**, 1.316.144 bytes, SHA-256
  `f17193a4b88f5839aa3af2521f32db93c2e3dc483b5a2b2e3a981526358a5b01`.
  Passou antes pela R6. Revisões, slots e digests conferidos após cada OTA.
  Mapa de 49 teclas/UART e `keyboard_ready=true` preservados. R7 aceita
  `READ6` com seis linhas compactadas e remove o rodapé de tokens de
  respostas antigas. `READ`/`READ4` legados mantidos. Relatórios privados:
  `device-r6-ota.json` e `device-r7-ota.json`.
- **STM32 permanece R8: R9 compilada, ainda NÃO instalada.** Nova R9 tem
  leitor em seis linhas, sem cabeçalho, ADC com diagnóstico e entrada direta
  no carregador residente, evitando depender da retenção do pedido na RAM
  durante reset. Aplicação 56.364 bytes, CRC `ced5235a`, pacote OTA 56.380
  bytes, SHA-256 `20a86cf2f0383f6782144db8122675d5ab81fb2628222fb88e0e08522e57c550`.
  Manifesto em `.build/lyke_stm32/lyke-r9/manifest.json`.
- A OTA do STM parou ANTES de apagar/gravar: `Tempo esgotado: LYKE BOOT;
  RX=MAP 49 1`. O STM reiniciou de volta à aplicação; usuário confirmou
  que MODE ainda abre o menu. Não repetir cegamente. O pedido de boot e
  o loader original foram conferidos nos fontes/binários históricos; a
  causa do retorno ao app ainda não foi comprovada no hardware.
- **Solicitado conectar o USB de dados do STM32** para inspecionar o
  carregador e instalar a R9 uma vez, preservando mapa/preferências.
  Até a última consulta não havia dispositivo `0483:df11` nem serial.
  Reidentificar alvo e ler loader/configuração antes de gravar. Último
  serial DFU histórico `209D35994333`; confirmar de novo. Depois validar
  uma OTA real da R9 para comprovar o novo caminho de entrada no loader.
- Bateria: usuário relata 6,66 V fixos; ainda sem multímetro. PA0/ADC1_IN1
  foi confirmado no datasheet. A fórmula com divisor 1:1 está coerente;
  6,66 V é compatível com leitura próxima ao fundo de escala de 3,33 V,
  mas não prova defeito/ligação. R9 rejeita saturação/faixa incompatível
  com uma célula, mostra PA0 mV/ADC bruto/VREFINT e usa espera/amostragem
  maior. Teste puro de conversão e paginação aprovados. Medir BAT+ e
  PA0 com GND comum no final, conforme pedido; não declarar bateria aferida.

**Manual do comprador preparado em 23/11/2026:** `output/pdf/Manual-Lyke.pdf`
(36 páginas), versão integral `.md`, pasta `manual-assets/` e pacote
`Manual-Lyke-Completo.zip`. Gerador em `scripts/manual_lyke/gerar_manual.py`.
Todas as telas são simulações R8/R5; a frente é desenho didático, e a foto
comercial é identificada como referência do produto original, sem alegar foto
da unidade modificada. Inclui tutoriais ilustrados de conta/crédito/chave
OpenAI e Claude. **Claude não está integrado nem testado:** o servidor atual
usa OpenAI Responses; trocar somente a chave não funciona. Para uma futura
edição pronta para venda, substituir/complementar com fotos da unidade final
e confirmar alimentação/recarga, três fotos e leitura real no LCD.
Nenhum firmware ou serviço foi alterado nesta tarefa de documentação.

**API e arquivo da Lyke concluídos em 23/11/2026:** ESP **Lyke R5** instalado
por OTA em **app1, `0x190000`**, com 49 teclas e `keyboard_ready=true` recebidos
do STM32 R8. Aplicação 1.315.616 bytes, SHA-256
`578de56bb213d127559a4f354622a9a5dff7880eae2e1fa408af2219a4deddc7`.
Digest em execução:
`befc58b61c7f070f423900b40605413e441a2e1e74ca0faaae3dd2d8ad14c2d7`.
Manifesto: `firmware/esp32cam_integrada/bin/lyke-r5/manifest.json`.
O AP abriu após o reboot e a estação conectou em DANIEL-HUAWEI. URL e
credencial exclusiva do aparelho foram gravadas no NVS. O ESP não guarda
agora a chave OpenAI do servidor. Tenta a última rede conectada ao iniciar,
com o AP de atualização preservado. A política de OFF da R4 foi mantida.

Organização Supabase **CAMERA_ESP**, projeto **camera-esp-lyke**, ref
`vxjmoctaomihrmxmuxip`, Free, sa-east-1. A conta aberta no Brave é diferente
da conexão MCP PUC_FILES; os novos recursos estão na conta do Brave.
Função `lyke-solve`, bucket privado `lyke-images`, tabela `lyke_requests`.
Modelo principal disponível: **GPT-6.1 Terra**, raciocínio alto; 6.2 Terra
não constava no catálogo consultado. Econômico GPT-6 Luna. Saldo OpenAI
US$ 10 foi confirmado antes dos testes e as chamadas reais foram aceitas.
Documentação completa em `docs/API-E-ARQUIVO.md`.

Validação: oito testes locais, consulta real de visão com equação sintética
`2*x+3=11` devolvendo `x=4`, cache idêntico no reenvio e JPEG baixado com
hash original. Bucket privado, RLS e ausência de leitura anon/authenticated
conferidos; auditoria Supabase terminou sem avisos.
**Teste completo do aparelho passou:** captura JPEG de 409.062 bytes,
envio pelo próprio ESP, armazenamento, resposta e última consulta arquivada
confirmados. A imagem era da bancada, sem exercício; o modelo pediu foto
do enunciado. A foto arquivada coincidiu com o JPEG original por SHA-256.
O leitor usa ASCII/21 colunas/quatro linhas; não foi aferida pelo usuário
uma resolução de questão real na tela nesta etapa. STM32 não foi gravado.

Repositório privado criado: `https://github.com/daniSoares08/camera-esp-lyke`.
Cópia de publicação sem credenciais em `.private/github_repo/`, preparada
por `scripts/prepare_private_repo.py`. A versão publicada contém fontes,
SQLs, testes, documentação e o esquema HTML offline; binários não entram.

Configuração e relatórios privados em `.private/lyke-cloud/`; senha do AP
em `.private/ap-password.txt`. Não publicar esses arquivos ou binários.
O usuário perguntou se podia desligar e foi informado que sim após o teste;
a conclusão da documentação/publicação não depende da calculadora ligada.
Capturas offline só entram no arquivo quando enviadas para resolução.

Pendências físicas: sessão de três fotos de questões reais, conferência no
LCD pelo usuário, alimentação definitiva/bateria aferida e OTA do STM32
via ESP. A resposta de rede e o mapa de 49 contatos já foram confirmados.

### Histórico de 22/11/2026 — instalação da R4

**ESP Lyke R4 gravada e testada em 22/11/2026:** após o usuário retirar
as ligações externas e colocar o ESP isolado no adaptador, a identificação
com reset automático respondeu em COM17 / FTDI `A5069RR4A`.
Confirmados ESP32-D0WD-V3 rev.3.1, MAC `68:09:47:9c:83:cc` e flash 4 MiB.
A leitura inicial a 115200 teve erro de dados; a 57600 foi concluída.
A tabela de partições e a seleção OTA foram conferidas. A aplicação R3
em app0 coincidiu com o binário conhecido e foi salva como cópia de
recuperação, junto do backup de partições/NVS/otadata.

R4 gravada **somente em app0, `0x10000`**, 1.312.608 bytes, SHA-256
`bc0032f79f7966114b64cfddec052c87c9cd8e6e83ff82e0f5efddf959adcdf5`.
O esptool conferiu o hash; os 32 KiB de tabela/NVS/otadata coincidiram
antes e depois. Manifesto: `firmware/esp32cam_integrada/bin/lyke-r4/manifest.json`.
Digest da imagem em execução:
`11143849e707b080e5d2954e638ff4c2d867ab6c6624c6b583e5d0ea6dd4c3b2`.
Relatórios e backup em `.private/esp32cam_lyke_serial_r4/`.

**Testes físicos passaram no ESP isolado:** boot da R4, portal automático
sem enviar OTA/WIFI, HTTP 200 em `Calc-Lyke` / `http://192.168.8.1`,
4 MiB de PSRAM, comando OFF de manutenção permanecendo desligado por
mais de 3 segundos e retorno automático do portal após reset.
Revisão/hash e página HTTP conferidos novamente após esse reset.
O portal ficou ligado ao encerrar; o PC foi desconectado do AP e a
internet por Ethernet foi preservada. Não foi preciso pressionar reset
manualmente durante as verificações finais.

STM32 permanece **R8**, com as 49 teclas salvas; não foi gravado nesta etapa.
O ESP isolado mostra `mapped_keys=0` e `keyboard_ready=false`, pois não há
UART com o STM nessa condição. Falta recolocar A2 → GPIO13, A3 ← GPIO14 e
GND comum, testar OFF pelo menu real do STM e ligar a montagem sem o
adaptador. Orientado usar apenas 5V + GND ao lado de 5V para alimentação,
com 3V3/VCC sem fios externos e GND comum ao STM. O pino ao lado de
U0T pode ser GND/R (reset) em outra variante: não aterrá-lo sem
identificação; a variante desta unidade e a ligação final seguem
pendentes. Captura, redes externas, API e bateria continuam pendentes.

### Histórico da preparação e das tentativas anteriores da R4

**Nova solicitação em 22/11/2026 — reinstalação do ESP:** usuário reconectou
o adaptador e determinou que o portal Wi-Fi fique ligado sem expiração,
desligando apenas por comando no STM; todo reinício do ESP deve reativá-lo.
Preparada e compilada a revisão **ESP Lyke R4**, 1.312.608 bytes,
SHA-256 `bc0032f79f7966114b64cfddec052c87c9cd8e6e83ff82e0f5efddf959adcdf5`.
Fontes em `firmware/esp32cam_integrada/lyke/`, manifesto em
`firmware/esp32cam_integrada/bin/lyke-r4/manifest.json`.
Mantém a intenção de Wi-Fi somente em RAM, tenta reabrir AP a cada 3 s
em caso de falha e permite OFF no modo OTA ocioso, protegendo transferência
em andamento. Buscar/conectar rede também abre o portal.
Teste nativo da lógica real de abertura/recuperação/OFF passou:
`.build/lyke_stm32/test_wifi_policy.py`; inclui ausência de expiração,
respeito a OFF, proteção da transferência e retorno do contador millis.
Esse teste usa Wi-Fi simulado e não comprova funcionamento físico.
**Antes da gravação R4:** adaptador FTDI `A5069RR4A` reenumerado em COM17,
mas STATUS e esptool não receberam dados. Usuário confirmou IO0 no GND
e RST; captura serial a 115200 terminou com zero bytes. Após retirar e
recolocar o USB com IO0 no GND, o FTDI reapareceu em COM17, mas o ESP
continuou sem resposta. Solicitadas medições atuais de 5V–GND e 3V3–GND
nos pinos do ESP. O usuário informou depois **VCC = 3,3 V, 3V3 = 3,3 V
e 5V = 5,01 V**; nova tentativa de identificação ROM sem reset ainda
resultou em `No serial data received`. Nenhuma escrita de flash
nesta tentativa até aqui. A tentativa a 9600 baud também não recebeu
dados. Solicitadas tensões IO0–GND e U0T/TX0–GND, com IO0 no GND,
e confirmação de eventuais fios do STM ainda em U0R/U0T.
Até essa etapa, a última instalação confirmada do ESP era Lyke R3; STM32 R8.

**Alimentação em diagnóstico (22/11/2026):** após o teste da R8, o usuário
relatou ausência de Calc-Lyke, “ESP SEM CONEXAO” e flash branco levemente
aceso. Confirmou que ligou a fonte de 5 V simultaneamente aos pinos
**5V e VCC do ESP32-CAM**, com GND comum ao STM. Foi solicitado desligar
e retirar a alimentação externa de VCC, mantendo apenas 5V e GND no ESP.
No esquema de referência V1.6, o pino VCC é selecionável entre 3V3 e 5V
por resistores de 0 ohm; a configuração da placa física não foi aferida.
Correção ainda não confirmada. Antes de novos testes de firmware, medir
5V–GND e 3V3–GND após corrigir a ligação; não há diagnóstico de dano.

Medição posterior informada pelo usuário: **VCC = 5 V e 3V3 = 3,24 V**.
Não há evidência de que os 5 V estejam aplicados diretamente ao domínio
3V3 nesta placa. A ausência de Wi-Fi permanece em diagnóstico; não atribuir
a falha a sobretensão como fato. Ainda sem confirmação de retirada do fio
externo em VCC. PC sem portas seriais detectadas nesta consulta. Solicitado
RST do ESP com IO0/BOOT solto e observação do AP antes de usar o menu.

O usuário pediu para salvar o estado e informou que vai organizar os
pinos definitivos. A montagem final ainda não foi confirmada. STM32
`lyke-r8` e ESP `lyke-r3` estão gravados e verificados. Não repetir o
mapeamento: 49 contatos estão salvos, ON foi dispensada e o erro entre
X^-1 e = foi corrigido. O mapa foi exportado da última leitura de flash
para [teclado-lyke.csv](teclado-lyke.csv), incluindo os GPIOs.

A pedido do usuário, foi gravada e conferida a revisão STM32
`lyke-r8`, 55.860 bytes de aplicação em `0x08002000`. Ela mantém o comando `PORTAL`
pela USART2 em A2/A3 a 115200, remove a operação temporária em CLK
e oferece nova tentativa com `=` e indicação de falta de resposta
após 5 segundos. A R3 instalada no ESP já atende esse comando.
SHA-256 da aplicação
`0eaaa824bbc5a9eb81997ce8692c7566c3dfd79f97449349712080f797ba7e28`.
As quatro páginas de configuração coincidiram antes e depois da gravação.
Leituras em `.private/stm32_lyke_r8/`. O STM32 saiu do DFU pelo USB.
Pacote para o portal em `.build/lyke_stm32/lyke-r8/lyke-stm32-ota.bin`.
Teste solicitado: desligar Wi-Fi pelo menu e abri-lo novamente por
MODE → Atualizar firmware, mantendo a UART na montagem final.
O usuário relatou “ESP SEM CONEXAO” e ausência de Calc-Lyke no celular.
Ainda falta observar a rede logo após ligar o conjunto, antes de usar
o menu de desligamento do Wi-Fi, e conferir a alimentação do próprio ESP.

Ligações finais de comunicação:

| Origem | Destino | Função |
|---|---|---|
| STM A2 / PA2 | ESP GPIO13 | TX do STM → RX do ESP |
| ESP GPIO14 | STM A3 / PA3 | TX do ESP → RX do STM |
| GND | GND | Referência comum |

Retirar o jumper temporário CLK/PA14 → U0R e soltar o fio de A3 do
U0T/TX0 antes de retorná-lo ao GPIO14. IO0 fica solto para iniciar o
firmware. O usuário informou que o ESP encaixa no adaptador conforme
o original; 3V3 não foi ligado. Essa descrição não confirma a topologia
de VCC nem um novo esquema de alimentação. O divisor de bateria é
470 kΩ + 470 kΩ em PA0, e a última condição informada era bateria solta.

No próximo teste, verificar no portal `Calc-Lyke`,
`http://192.168.8.1`, se `mapped_keys=49` e `keyboard_ready=true`.
Usar a senha fixa local já acordada. O menu MODE permite abrir
configuração/atualização. A R3 do ESP abre o portal automaticamente no
boot, sem comando serial. Isso foi confirmado após o reinício da OTA;
falta o teste de alimentação fria com os conectores de manutenção fora.
Com a UART confirmada, testar a OTA do STM32 via o portal e conferir que
o mapa permaneceu salvo.

Wi-Fi externo e chave da API ainda estão vazios no namespace Lyke.
O último `/info` confirmou `key_set=false`, `saved=[]` e `station=""`.
Configurar pela página do celular usando as credenciais privadas
existentes, sem publicá-las. Captura fica bloqueada até o ESP receber
o mapa concluído do STM. Não enviar consulta paga para teste sem ação
explícita do usuário.

Depois testar busca/conexão de rede, 1–3 capturas locais, foco,
standby, galeria, consulta explícita à IA e tensão de bateria aferida.
MODE abre menu, AC volta/limpa, SHIFT + direita captura, SHIFT + AC
apaga o visor. O usuário desliga o conjunto pela chave da bateria.

Os manifestos e hashes estão na seção `lyke_current` de
[inventario.json](inventario.json). O relatório da OTA está em
`.private/esp32cam_lyke_migration/result.json`. O backup do mapa
concluído está em `.private/stm32_lyke_r7/configuration-after.bin`.

## Retomada do ESP · 22/11/2026

O usuário ressoldou pinos e informou encaixe original com o adaptador
FTDI, incluindo VCC e 5V, mantendo 3V3 sem ligação. COM17/FTDI
`A5069RR4A` reidentificado. Pela primeira vez nesta retomada houve
resposta do ESP: ROM em modo normal `0x12`, depois `STATUS` confirmou
`cam-ota-r4-clean`, ESP32-D0WD-V3 rev.3.1, 4 MiB de flash e PSRAM,
app1 e digest da imagem original. O comando `OTA` respondeu
`EspCam-968 http://192.168.8.1`, e o portal confirmou OTA ativo.

O índice atual do Wi-Fi do PC é 19, Ethernet 9. O script temporário
`.build/lyke_stm32/migrate_esp_from_stm.py` passou a resolver o índice
do Wi-Fi dinamicamente. A primeira conexão caiu antes do envio. Após
retirar e recolocar o USB com IO0 solto, o ESP respondeu novamente ao
comando OTA.

**ESP Lyke R3 instalada por Wi-Fi e verificada após reinício em app0,
`0x10000`**, 1.312.064 bytes. SHA-256 do arquivo
`96a75314bdea88db2fa9a991ca359b7fa1c1a41eee1ba41c03663ff39a16e209`.
Digest informado pela imagem em execução
`0691d72b07c3aa17e4919b07de8b0a0622a6ba0e5504740a3e4e4c08b201ded9`.
O novo AP `Calc-Lyke` abriu automaticamente após o reinício. Portal
HTTP 200 com opções para atualizar ESP e STM32. Evidência privada em
`.private/esp32cam_lyke_migration/result.json`, sem token.

Memória com portal ligado e câmera desligada: PSRAM livre 4.183.120 bytes,
heap livre 161.140 bytes, maior bloco interno 110.580 bytes. Ainda sem
teste de captura. O portal indicou zero teclas recebidas pela UART,
compatível com o retorno A3 ainda em TX0 do diagnóstico ou STM desligado.
Foi solicitado retirar CLK → U0R e restabelecer A3 ← GPIO14, A2 → GPIO13,
GND comum, com a alimentação desligada, e depois ligar as duas placas.

## Lyke 101078 integrada · 16/11/2026

O usuário informou que STM32G431CBU6, ESP32-CAM, teclado e LCD estão conectados.
O divisor de 2 × 470 kΩ chega a PA0 e a bateria está desconectada. As fotos
mostram as 50 teclas físicas e parte do LCD sob a moldura. A revisão Lyke
usa 18 colunas nas cinco linhas centrais, contraste 5 e rotação a 180°.

Os dois firmwares `lyke-r1` foram compilados. A revisão inicial do STM32
abre o mapeamento guiado e salva cada par na flash. A conclusão libera
a calculadora local. O ESP clássico serve o portal fixo `Calc-Lyke`,
Wi-Fi, galeria e a consulta em lote de uma a três fotos. A instalação
inicial do STM32 instala o carregador residente por DFU e a OTA do
STM32 passa pelo ESP via UART. Fontes, arquivos e fluxo no
[guia da Lyke](../../firmware/lyke_stm32/LEIA-ME.md).

STM32 `lyke-r1` gravada por USB DFU no serial `209D35994333` e
conferida por leitura de 63.212 bytes desde `0x08000000`. SHA-256
`2b843c988bb3cc7489541b0673bc015811c8df5ff5f6f22f536244a6bcc58892`.
A leitura de 128 KiB anterior à instalação confirmou o mapeador R4 e
fica em `.private/stm32_lyke_initial/`. O comando de saída do DFU
encerrou a enumeração USB. O usuário confirmou a primeira página
legível e identificou que o recorte da carcaça limita somente o topo.
A Lyke R2 usa a largura original de 21 colunas nas seis linhas
inferiores. O leitor tem quatro linhas por página, com compatibilidade
do ESP R2 com o leitor de três linhas do STM R1.

STM32 `lyke-r2` gravada e verificada no mesmo dispositivo por DFU,
somente na área de aplicação, 54.940 bytes desde `0x08002000`.
SHA-256 da aplicação
`719b8e6b0195e946574d5056e088afd145b3d9f3b7e307b76ca1053d43f1f6b2`.
As quatro páginas de configuração coincidiram antes e depois; havia
7 pares válidos, portanto a próxima tecla é ON (08/50). As leituras
estão em `.private/stm32_lyke_r2/`. O STM32 saiu do DFU pelo USB.
O usuário confirmou que as laterais do LCD aparecem inteiras na R2.
Na etapa 08/50, segurar ON não alterou o aviso “UMA TECLA POR VEZ”.
MODE, que já tinha par salvo, retornou “TECLA JA MAPEADA”, confirmando
que a varredura está ativa. A inicialização e a varredura dos GPIOs são
idênticas às do mapeador R4. O usuário optou por dispensar ON, pois usa
a chave física da bateria.

STM32 `lyke-r3` gravada e verificada por DFU no mesmo dispositivo,
somente na área de aplicação, 55.372 bytes desde `0x08002000`.
SHA-256 da aplicação
`06dabcb4889fb8c8ceb1d7afa4225e4c65118039a3323d114f2239afbcd6c930`.
As quatro páginas de configuração coincidiram na leitura antes e depois,
com os sete contatos já salvos. O STM32 saiu do DFU. A R3 pula ON no
boot e deve retomar em X^-1, 08/49. A conclusão ocorre após 49 pares.
O atalho para captura é SHIFT + REPLAY direita. A aplicação do ESP R3
foi compilada e aceita o mapa de 49 ou 50 teclas pela UART.

O mapeamento chegou a 48 contatos e ao registro de pulo ON. Na última
tecla, o usuário informou que `=` havia sido associado a uma tecla
anterior e que o último passo mostrava contato já mapeado. A leitura
de flash mostrou `ab/c` em T14–T19 e `X^-1` em T09–T13. Pelo
padrão das teclas vizinhas, T09–T13 provavelmente corresponde a `=`,
mas a confirmação depende do toque físico.

A R4 foi gravada e lida de volta em DFU. Antes de sair do DFU, foi
substituída pela R5, que corrige qualquer associação com `=` durante a
última etapa. A R5 foi gravada apenas em `0x08002000` e conferida por leitura,
55.764 bytes, SHA-256
`9801075135c00aef167ab59e6d6523ac28dbad10ded7168a3d0b7744e3fbca0f`.
As quatro páginas de configuração coincidiram antes e depois. O STM32
saiu do DFU. A R5 conserva o mapa até receber o contato da tecla correta,
quando grava um único registro que restaura as duas associações. A revisão
do ESP ainda preparada é a R3.

O usuário confirmou que o mapeamento das 49 teclas terminou na R5.
O valor mostrado no teste de fração ainda não foi informado.

Naquele momento estava pendente instalar a Lyke R3 no ESP clássico, testar OTA
STM32, medir a bateria
com multímetro e verificar Wi-Fi, memória disponível, fotos e API.
O ESP ficou sem o conector serial soldado. Seu último firmware confirmado
é a R4 de bancada, que inicia com rádio desligado e recebe o comando
`OTA` pela UART0, antes da transferência Wi-Fi. A UART entre ESP e STM
só passa a interoperar depois de instalar o perfil Lyke no ESP.

Para abrir a OTA legada sem conversor, foi compilada a R6 do STM32.
Usa USART2 TX temporariamente em PA14 (CLK no conector SWD, AF7) a
460800 baud, por jumper ao U0R/GPIO3 do ESP com GND comum. Em
**MODE → Atualizar firmware**, confirmar a opção de abrir o ESP antigo
com `=`. A UART permanente de A2/A3 permanece conectada para a R3 do ESP.
R6 gravada apenas em `0x08002000` e conferida por leitura, 56.116 bytes,
SHA-256 `dce96918234bd722d8f1d46ce919ebbaaa87eec14541348e4748a6c7a311938f`.
A área de configuração de 8 KiB foi idêntica antes e depois, com 49
contatos, pulo ON e reparo na posição X^-1. O STM32 saiu do modo DFU.
O usuário ligou CLK a U0R e confirmou “COMANDO OTA ENVIADO” no LCD. O
PC tentou associar ao SSID legado esperado e o Windows informou que a
rede não estava disponível. Após nova tentativa, o usuário confirmou
que a rede também não aparece no celular. Não houve transferência para
o ESP. O usuário mediu 4,8 V em 5V e 3,2 V em U0R em relação ao GND
do ESP. O usuário confirmou continuidade e reenviou após religar a
montagem. Uma nova associação do PC expirou esperando o AP legado.
Foi compilada a R7 do STM32 para verificar a resposta no LCD: mantém
CLK/PA14 → U0R/GPIO3 e, temporariamente, recebe por A3/PA3 ← U0T/GPIO1,
com o fio de A3 solto do GPIO14 durante o diagnóstico. O usuário relatou
que moveu A3 de GPIO14 para TX0. A R7 foi gravada por DFU no STM32 de serial
`209D35994333`, somente na aplicação em `0x08002000`, e verificada por
leitura de 56.404 bytes, SHA-256
`64373cb03ae54362c44a55fae4583cee626d7ee6f334f07f37ac14cc874c76c0`.
As páginas de configuração coincidiram antes e depois. O STM32 saiu do
DFU. Com o retorno físico informado em TX0, o teste da R7 exibiu
“SEM RESPOSTA UART”. O adaptador FTDI `A5069RR4A` foi reidentificado como
COM17 com TX → U0R, RX → U0T e GND comum. O comando `STATUS` a 460800
expirou. O usuário acionou RST do ESP durante uma captura a 115200 e
nenhum byte foi recebido. `esptool read-mac` sem reset também não recebeu
dados. O usuário informou uma leitura de 3,2 V, sem indicar qual dos
dois pinos foi medido. Foi solicitado o teste de leitura no modo ROM
(IO0 aterrado ao soltar RST), mantendo a alimentação do ESP separada
do VCC do FTDI e GND em comum. Ainda falta a confirmação de que o ESP
estava alimentado e com IO0 aterrado durante uma tentativa. Nenhuma
gravação no ESP foi feita.

## Mapeador de bancada STM32 R4

O usuário escolheu o nível 5 do LCD e relatou que todas as teclas mostraram
alguma resposta em teste fora de ordem. Os pares por tecla e sua unicidade
aguardam mapeamento ordenado.

Gravado por USB DFU e conferido por leitura `mapeador-r4` em `0x08000000`,
3.432 bytes, serial USB `209D35994333`, flash interna de 128 KiB.
SHA-256 `1d1613df52b2221107a9ad8c19b425267c1631feb69e33bfeb7c8f32ad59ef02`.
A R4 aplica na inicialização os mesmos comandos do quinto nível da R3
(`26 81 14` em hexadecimal). Rotação a 180° e varredura mantidas. O usuário
confirmou que a tela ficou legível após reiniciar. Houve alguns toques
acidentais. Reiniciar antes da rodada ordenada para limpar os pares em RAM.

## Histórico do mapeador de bancada STM32 R3

O usuário confirmou teclado no STM32 com T19 em PC6 e LED D1 removido,
PA11/PA12 livres, LCD conectado conforme o HTML, ESP e bateria desconectados.
O LED vermelho de alimentação estava aceso por USB. Entrou em DFU com
BOOT0 + NRST. WinUSB instalado para `0483:DF11`, serial `209D35994333`.
A interface 0 informou 128 KiB de flash em `0x08000000`, 64 páginas de 2 KiB.

Gravado e conferido por leitura `mapeador-r3` em `0x08000000`, 3.576 bytes.
SHA-256 `1cdf1da49e558f975010ba40615a4d1f6de4d5082866b704104f15c1be0497f8`.
A R1 (3.396 bytes) foi gravada e verificada antes, mas o LCD ficou muito
escuro e o usuário só viu texto olhando de lado. O usuário confirmou que a
R2 (3.568 bytes) ofereceu contraste legível de frente. A R3 conserva os seis
níveis percorridos até o primeiro par e gira a imagem 180°. Preservada cópia
de 128 KiB da flash anterior em `.private/stm32_pre_mapeador/`. O teste de
lógica verificou contatos bidirecionais, duplicatas, debounce e rejeição de
múltiplos contatos. O usuário confirmou o teste inicial da R3 e depois
relatou resposta visual de todas as teclas, sem registrar os pares em ordem.
Fontes e instruções em
[`firmware/stm32_teclado`](../../firmware/stm32_teclado/LEIA-ME.md).

## Pinagem física do teclado — 15/11/2026

O HTML e seu JSON agrupam as trilhas em 16 pinos na borda inferior do STM32
(lado B13/B12) e 3 na superior, com USB-C à esquerda. Para liberar essa borda,
o LCD passou para SPI1 e a UART para USART2, ambos na borda superior. O ADC
continua em PA0. Essa revisão substituiu a proposta de 13 fios em cima e 6 embaixo.
Na montagem confirmada no mapeador, T19 foi movida de B0 para C6 (17 embaixo,
duas em cima).

## Teclado novo e esquema — 14/11/2026

Variante recomendada para minimizar os fios no ESP: STM32G431CBU6 para
teclado de 19 trilhas, tela, ADC da bateria e lógica offline, com ESP32-CAM
para câmera, Wi-Fi, API e OTA. UART por GPIO13/14. Ligação e firmware ainda
em fase de proposta. Consulte a seção STM32 no [guia de pinagem](TECLADO-19.md).

Nova calculadora com 19 trilhas no cabo do teclado original. O HTML principal
mostra STM32 + ESP32-CAM. As propostas anteriores com PCF8575 são históricas.
O FT4232H Mini Module Rev. 1.1 da foto pode servir de instrumento de bancada
com o PC como host. O ESP32 clássico carece de USB Host. O usuário prefere
dispensar o FTDI da montagem e usar a rede para manutenção do ESP.

Consulte [Teclado original de 19 trilhas](TECLADO-19.md) para ligações,
condições da medição e pendências. A última gravação confirmada continua a
revisão r4 de 09/11, cujo acionamento de OTA depende hoje do comando serial.

## ESP32-CAM clássica: atual cam-ota-r4-clean — 09/11/2026

Depois de dessoldar o LED, o usuário reconectou o ESP e pediu retirar o teste.
COM17/FTDI A5069RR4A reidentificados; a placa respondeu como ESP32-D0WD-V3,
flash 4 MiB, executando r3 em app0, ADC de bateria desabilitado.

**Atual: `cam-ota-r4-clean`, instalada por OTA em app1 (0x190000)**.
1.146.544 bytes, 426.320 bytes livres no slot de 1,5 MiB; envio por Wi-Fi
em 5,77 s, com revisão/slot/digest conferidos após reiniciar. Foram removidos
comandos LED33, temporizador, função de piscadas e campo de status do teste.
GPIO33 inicia como INPUT sem pull; não é acionado como saída/LED pelo fonte.
ADC de bateria continua desabilitado até comando explícito. Sem mudanças de
partições ou ligações; pad/fio/divisor seguem sem confirmação física.

Wi-Fi do ESP encerrado, perfil temporário removido e rádio Wi-Fi do PC
restaurado desligado, preservando Ethernet. Evidências:
`outputs/esp32cam_ota/update_sem_led.json` e `fontes_sem_led.json`;
release/manifesto em `firmware/esp32cam_integrada/bin/cam-ota-r4-clean/`.

## Histórico: teste temporário cam-ota-r3-led33 — 09/11/2026

Atualização posterior a r2, a pedido do usuário para identificar visualmente
o LED vermelho antes de removê-lo. COM17/FTDI A5069RR4A reidentificados;
status confirmou ESP32-D0WD-V3 e flash/PSRAM 4 MiB, ADC de bateria desligado.
Nova versão **`cam-ota-r3-led33` instalada por Wi-Fi, app0 (0x10000)**,
1.147.136 bytes, 425.728 bytes livres no slot de 1,5 MiB. Envio em 5,78 s,
reinício/versão/slot/digest conferidos. Evidência:
`outputs/esp32cam_ota/update_led33.json`; manifesto e binários locais em
`firmware/esp32cam_integrada/bin/cam-ota-r3-led33/`.

Comando `LED33 BLINK`: três pulsos de 250 ms, separados por 250 ms, repetidos
a cada 3 s, ativo em LOW. Teste dura 180 s; ao terminar, GPIO33 volta a INPUT
sem pull. `LED33 OFF` encerra imediatamente. Não ativa no boot. Se o ADC da
bateria estiver habilitado, o comando de piscar é recusado; `ADC ON` encerra
o teste antes de reservar o pino para leitura. Flash branco/GPIO4 não participa.
Após o teste, o usuário informou: **dessoldou o LED**. Registrar a retirada
do LED vermelho como confirmação do usuário; pad do GPIO33, solda do fio e
montagem do divisor ainda não foram confirmados por medição. Na consulta
seguinte, nenhuma porta serial foi detectada, portanto não foi enviado
`LED33 OFF`. O teste expira em 180 s e não reinicia automaticamente no boot.
A retirada do LED não altera o funcionamento do OTA.

Wi-Fi do ESP encerrado após o update; rádio Wi-Fi do PC restaurado desligado,
perfil temporário removido, Ethernet preservado. `update_once.py` fornece
envio OTA único com verificação de manifesto e conferência após boot.

## Histórico da implementação e teste OTA r2 — 09/11/2026

Usuário autorizou implementar, otimizar, gravar por USB e testar OTA, mesmo
sem câmera, LCD, teclado ou demais periféricos conectados. Isso substitui a
restrição histórica de apenas preservar documentação. Nenhuma ligação mudou.

- **Atual: `cam-ota-r2-wifi`, projeto `esp32cam_integrada`, rodando em app1
  (0x190000)**. Binário de 1.146.528 bytes = 1,0934 MiB, 72,89% do slot de
  1,5 MiB, com 426.336 bytes livres. SHA-256 do arquivo:
  `04ba55dff4a961ec00c08029f9207a6fe931101e81442b0f791fed3362b0e17e`.
- Primeira instalação por USB: `cam-ota-r1-usb`, app0, 1.178.000 bytes;
  aplicativo e tabela conferidos por esptool após gravar. Segundo envio foi
  **HTTP pelo Wi-Fi**, 5,7 s; não foi uma segunda gravação USB. Conferidos
  reinício, app1, revisão, digest da imagem e resposta HTTP da nova versão.
- Dois slots OTA reais de 1,5 MiB. NVS preservada em 0x9000; área de arquivos
  mantida em 0x310000, 896 KiB. LittleFS formatado explicitamente no teste.
  Antes da migração, foram preservados 64 KiB de boot/tabela/NVS em
  `.private/esp32cam_pre_ota/boot-particoes-nvs.bin`. Tentativas de backup
  integral falharam na leitura serial; **não existe backup integral validado**.
  O aplicativo original r6 e manifesto continuam em `firmware/release_esp32cam`.
- Nova medição direta confirmou PSRAM 4.194.304 bytes. Estado final sem
  periféricos: PSRAM livre 4.188.548 bytes, heap livre 205.488 bytes, maior
  bloco interno 110.580 bytes. Esses valores não representam consumo com
  câmera/tela/API em uso. Câmera ausente retorna 106 e não impede o OTA.
- Teste de armazenamento sem sensor: quatro cópias de JPEG real de 498.378
  bytes, três em PSRAM e uma na flash; quinto recusado por limite; todos os
  downloads conferidos por hash. Após reboot, só a foto da flash permaneceu.
  Entrar no OTA apagou todas as fotos, como aceito pelo usuário.
- Verificadas recusas de autorização, nome/chip/projeto incorretos, tamanho
  excedido, truncamento, corrupção e SHA-256 incorreto. Todos preservaram a
  versão em execução. Rollback automático por falha funcional **não habilitado**.
- Otimização: `-Os` + LTO do C++; economia de 31.472 bytes (2,672%) em relação
  à primeira compilação. LTO também no C falhou por relocação Xtensa no core;
  manteve-se C em `-Os`, sem ignorar o erro do linker. A referência r1 tinha
  PCF no I2C1; r2 corrigiu para I2C0, com os mesmos GPIO13/14, pois o SCCB da
  câmera usa **I2C1 neste SDK 3.3.6**, confirmado em sdkconfig e no diagnóstico.
- Base inclui câmera/AF r6, fila PSRAM/LittleFS, galeria/OTA HTTP local,
  núcleo matemático e drivers preliminares PCF/LCD/matriz. **Não inclui ainda
  HTTPS/API, provisionamento, menu completo ou mapa físico do teclado 4×5**.
  Não somar essa medição ao tamanho da calculadora S3 como se fossem o mesmo
  firmware. LCD e ADC são habilitados explicitamente; ADC33 fica desabilitado.
- UART 460800, DTR/RTS desativados. `STATUS` diagnostica; `WIFI` abre galeria;
  `OTA` apaga fotos e abre atualização; `OFF` desliga Wi-Fi. AP `EspCam-…`,
  senha local solicitada `CHANGE_ME_ESP_AP`, `http://192.168.8.1`. Só abre por comando;
  reiniciar volta com Wi-Fi desligado. Upload somente `esp32cam_integrada.ino.bin`.
- Wi-Fi do computador restaurado a rádio desligado por software e sem o
  perfil temporário; Ethernet permaneceu ativo e preferencial durante o teste.
  No ESP, Wi-Fi desligado, importação de bancada desabilitada e fila vazia.
- Fontes/scripts/limites: `firmware/esp32cam_integrada/LEIA-ME.md`.
  Evidências: `outputs/esp32cam_ota/{verificacao,estado_final,otimizacao,fontes}.json`.

## Histórico anterior: firmware de câmera r6

Inspeção adicional de 09/11: COM17 reidentificada, ESP32-D0WD-V3 rev.3.1 e
flash 4 MiB confirmados; aplicativo r6 conferido por `verify-flash`, sem
gravação. Usuário informa placa sem câmera/periféricos; consulta retorna
`CAMERA_ERROR 106`, sem telemetria de memória livre atual. PSRAM de 4 MiB
permanece sustentada pela medição anterior. Estimativa e limites em
[Memória e fotos](MEMORIA-E-FOTOS-2026-11-09.md). Não houve integração de Wi-Fi,
LCD ou teclado; nenhuma pendência dessa integração foi concluída.

- Identificada por ferramenta: **ESP32-D0WD-V3, revisão 3.1**, flash 4 MiB e
  PSRAM 4 MiB; perfil de pinos AI Thinker. Não é S3, apesar do anúncio/nome
  inicialmente informado. COM17 histórica, conversor FTDI VID0403/PID6001,
  serial A5069RR4A. Reidentificar se necessário.
- Versão gravada e verificada: **uart-camera-esp32cam-20261108-r6**.
- Fonte: `firmware/camera_usb/serial_camera_usb.ino`, com seleção condicional
  para ESP32 clássico ou S3; o mesmo fonte não implica o mesmo binário.
- Aplicativo: `firmware/release_esp32cam/serial_camera_usb.ino.bin`, 405168 bytes,
  SHA-256 `660b499312e90f2b43d7b5ceef2fb749e6677a51e3b350312777b88d71189d69`.
  Manifesto completo nessa pasta. Gravar clássico com `--chip esp32`, flash
  4 MB e bootloader em 0x1000; não usar os binários S3 de `release_pc`.
- UART 460800 baud; S3 USB nativa 115200. Este firmware é de bancada,
  **sem Wi-Fi e sem API**, e mantém câmera inicializada para prévia/foco.
- Autofoco OV5640: carrega firmware AF, oferece foco único, contínuo e pausa.
  Foco manual não validado foi desativado. Estado 0x10 sozinho indica término,
  não prova de foco: consultar resultado da zona central, esperado 0.
- Foi observado ACK=1 / AF=0x20 travado; reiniciar liberou. A origem do estado
  não foi demonstrada. r6 faz recuperação limitada, espera ACK e distingue
  erro de comunicação de busca sem foco. O painel mantém erro visível.
- Bancada: dois focos sucessivos QSXGA e um VGA concluíram com zona central 0,
  ACK 0; foco contínuo também respondeu; JPEG decodificado. Evidências em
  `outputs/teste_esp32cam_2026-11-08/verificacao_af_r6.json` e arquivos próximos.
  Contratos em `outputs/tests/test_camera_focus.py`.

### Abrir o programa da câmera

- `Abrir camera.bat` na raiz ou atalho `C:\Users\campo\Desktop\ESP-CAM.lnk`.
- Painel: `outputs/controle_camera.py`; protocolo: `outputs/camera_usb.py`.
- Alternativa: `.venv\Scripts\python.exe outputs\controle_camera.py --port COM17 --connect`.
- Atalho tem nome **ESP-CAM**, ícone `outputs/assets/esp-cam.ico`, usa pythonw.
- Tirar foto / Espaço salva JPEG original e metadados em `outputs/fotos`.
  Ver em 100% examina os pixels originais. A prévia reduz para caber e gira
  sem cortar/esticar: 4:3 torna-se 3:4 ao girar 90°/270°. Enquadrar a folha
  inteira exige distância/posição adequadas. Perfil em `outputs/perfil_documento.json`.
- Fechar outros monitores/painéis antes de usar a mesma porta serial.

## Calculadora S3 + DevKit: implementação e entrega

Fontes `firmware/calculadora/s3_calculadora/` e `devkit_display/`; elementos
comuns em `common/`. Não substituir pela aplicação de câmera de bancada.

- S3: GOOUUU S3-CAM N16R8, 16 MB flash / 8 MB OPI PSRAM. Usa USB/UART,
  pois GPIO19/20 são do teclado. Não usar OTG nessa montagem.
- DevKit V1 / ESP32-WROOM-32, flash 4 MB. Versão identificada no fonte:
  `devkit-display-v5`, LCD 180°, CS ativos LOW, DB1=14, DB2=13.
- Calculadora algébrica por padrão; RPN opcional; não emula ROM/RPL/CAS HP48.
- E abre menu; SPC captura 1–3 fotos; envio explícito pelo menu. Cabeçalho
  mostra três quadradinhos (vazio, piscando durante captura, cheio ao concluir)
  e pequeno ícone Wi-Fi só quando conectado. π como símbolo; apagar remove
  funções/constantes como tokens. Respostas roláveis nas setas.
- Câmera de foco fixo da calculadora não pode impedir captura por falta de AF.
  Inicializar apenas para capturar, copiar JPEG para PSRAM e retornar a standby
  antes de HTTPS. Standby/software power-down + XCLK parado não equivalem a
  corte físico de alimentação. Não houve medição de corrente.
- Consulta direta à Responses API, chave própria configurada no aparelho;
  não usa login/cookies do ChatGPT, nem insere automaticamente fotos neste chat.
  Não repetir envio pago automaticamente. Captura/visualização local não gasta API.
  Modelos/preços escritos em documentos antigos são históricos: confirmar
  disponibilidade e preços oficiais antes de mudar essa integração.
- Galeria `/foto` deve permitir selecionar/baixar imagens 1/2/3. Fotos em RAM
  desaparecem ao desligar/reiniciar; isso foi confirmado pelo usuário.
- Wi-Fi de casa DANIEL-HUAWEI funcionou. IPHONEPROMAX foi encontrado na busca,
  mas conexão não foi confirmada, mesmo com compatibilidade do iPhone ativada.
- Última revisão final do S3 de **03/11 ainda pendente de envio confirmado**:
  bateria acabou antes de prosseguir. DevKit aceitou OTA nessa etapa, porém
  slot/versão após reinício ainda precisava ser conferido.
- Pacote: `outputs/calculadora/firmware_wifi_2026-11-03.zip`; detalhes e
  procedimento em `firmware/calculadora/PRONTO-PARA-ENVIAR.md`.
- Binários correntes em `firmware/calculadora/bin/{s3_calculadora,devkit_display}`,
  com manifestos. Hashes e tamanhos fotografados em `inventario.json` desta base.
- Scripts locais: `Compilar.ps1`, `Testar.ps1`, `Gravar.ps1` nessa pasta.
  Slots OTA: S3 3 MiB cada; DevKit 1,5 MiB cada. Instalação inicial das
  partições por USB, depois OTA apenas aplicativo `.ino.bin`, nunca merged,
  bootloader ou tabela de partições. Não deduzir layout apenas do nome FQBN.

### Acesso por Wi-Fi à calculadora

- Senha fixa solicitada: **CHANGE_ME_ESP_AP**.
- Atualização S3: `CalcS3-8428`; tela: `CalcTela-1d44` (sufixos históricos).
- Galeria: `CalcFoto-8428`, `http://192.168.8.1/foto`.
- Atualização: E → Atualizar firmware → alvo; `http://192.168.8.1`.
  Sem limite de dez minutos enquanto estiver nessa opção.
- Configuração pelo celular: `CalcSetup-…`; tem ciclo próprio documentado,
  não confundir seu prazo com OTA sem expiração.
- Internet do PC pelo cabo Ethernet; Wi-Fi apenas para falar com a calculadora,
  conforme solicitação expressa do usuário. Não substituir rota de internet
  pelo AP da calculadora, que não fornece internet.
- Chave API/perfis privados: `firmware/calculadora/secrets.local.h` quando
  existente, configuração USB/celular e NVS. Não registrar chave nesta base.
  SSID antigo do celular foi substituído por **IPHONEPROMAX**. Não restaurar
  o nome antigo com espaços só porque aparece no histórico.

## Histórico de 09/11: esquema 4×5 da futura integração

Atualizado e verificado em navegador desktop e larguras 390/320 px:
versão de 09/11 do `outputs/ligacoes/ESP-CAM-esquema.html`. Incluía desenhos de ESP, LCD, placa
convertida PCF8575 e teclado; filtros e seleção de fios/pinos. Bateria adicionada
em 09/11: GPIO33/ADC1_CH5, 2 × 470 kΩ e 100 nF, retirada do LED vermelho.
Plano tem 19 grupos de conexão, nove fios de matriz e quatro portas PCF livres.
Sem erros JS nas verificações. PNG/SVG/guia/JSON também atualizados.

Fonte HTML: `outputs/ligacoes/esquema-esp-cam.template.html`. Geradores:
1. `.venv\Scripts\python.exe outputs\ligacoes\gerar_esquema_geral.py`
2. `.venv\Scripts\python.exe outputs\ligacoes\gerar_html.py`

O primeiro regrava o plano JSON, guia e pôster; editar o gerador para manter
mudanças duráveis. O segundo embute JSON no HTML independente de internet.
Prévia local: `python -m http.server 8768 --bind 127.0.0.1 --directory outputs/ligacoes`.
O processo pode não continuar em outro chat; reiniciar se necessário.
Verificação UI existente: `output/playwright/check-esp-cam.cjs`.

### Pendências que não podem ser tratadas como concluídas

1. Confirmar montagem física da placa convertida PCF8575, LCD, teclado 4×5 e
   bateria. Pinagem do cabo do teclado 4×5 ainda deve ser identificada.
2. Validar fisicamente a base nova **câmera + PCF + LCD SPI + teclado + ADC**.
   Drivers e OTA estão implementados; não houve teste conjunto com periféricos.
   Associar as posições físicas da matriz às ações, implementar menu completo,
   provisionamento e HTTPS/API; medir novamente o tamanho antes de fechar limites.
3. Liberar GPIO33 do LED vermelho e conferir pad real por continuidade.
   Divisor está planejado/documentado, sem prova de montagem/leitura física.
4. Concluir OTA pendente do S3 antigo, conferir DevKit após boot, diagnosticar
   hotspot iPhone e testar consulta multimagem quando o usuário retomar isso.
5. Confirmar foco/nitidez em questões reais; sucesso do comando AF não garante
   legibilidade do documento. Não diagnosticar defeito óptico só por OCR ruim.

## Ferramentas existentes

Windows/PowerShell; `.venv` Python com pyserial/Pillow; Arduino CLI 1.5.1 em
`.tools/arduino-cli-stable`; core ESP32 3.3.6; U8g2 2.36.19; ArduinoJson 7.4.2;
esptool; Node/npx para verificação da interface. Confirmar instalação antes
de usar em outro PC. Não é necessário atualizar dependências para retomar.
