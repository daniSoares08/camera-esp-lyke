# Base de conhecimento — CAMERA_ESP

Atualização: **25/11/2026**, America/Sao_Paulo. Raiz atual:
`C:\Projects\CAMERA_ESP`. Esta é a memória local do projeto para continuar em
outro chat; não é uma garantia de memória automática fora desta pasta.

## Começar por aqui

**Pedido atual de 25/11:** respostas prolixas, símbolos e quebras confusas;
reinícios ao fotografar com Wi-Fi, calor e bateria curta. API v4 publicada.
**ESP R10 preparada, não instalada:** usuário pediu aguardar porque a bateria
segue crítica. Instalados continuam STM R12 + ESP R9a. Só o ESP precisa
atualizar. Guia: `docs/REVISAO-ESP-R10-ENERGIA-E-RESPOSTAS.md`.
Usuário autorizou trocar portal permanente por portal no boot/menu e economia
no uso normal. Resistor de 330 Ω em +5 V/GND confirmado como carga mínima do boost.
Onze fotos/cinco consultas recuperadas para download privado; uma consulta paga
de regressão com foto existente. Não publicar fotos/credenciais/binários.

**Wi-Fi, ainda em 24/11:** roteador do celular salvo e conexão confirmada
no ESP R9a em duas leituras separadas por 47 s. Visitantes encontrada
na busca 2,4 GHz, mas conexão não confirmada; usuário priorizou o celular.
Sem alteração do firmware instalado nem publicação de senha. Ver o estado.

**Instalação de 24/11:** usuário ligou e autorizou a atualização. Instalados
**STM R12 + ESP R9a**, por Wi-Fi/UART existente, sem novos fios/USB.
Revisões, reinício, 49 teclas e portal conferidos; configurações e última
resposta preservadas. API v3 publicada; novo aviso Wi-Fi e desenhos roláveis.
Guia em `docs/REVISAO-R12-DESENHOS-E-WIFI.md`.

Manual edição 3.0 em PDF/MD em `docs/manual/` e `output/pdf/`.
A API retornou figuras nas consultas de 24/11; rolagem/desenhos no LCD
e conexão Visitantes continuam sem confirmação física específica. Bateria normal/ícone e
notação na janela original já haviam sido confirmados pelo usuário na R11.
Repositório público; credenciais, fotos de questões e binários privados.
O encerramento de 23/11 é histórico; houve novo pedido em 24/11.

1. [Estado, firmwares e próximos passos](ESTADO-E-RETOMADA.md)
2. [Componentes, pinagens e alimentação](HARDWARE.md)
3. [Preferências e decisões da conversa](DECISOES.md)
4. [Inventário verificável de arquivos e hashes](inventario.json)
5. [Mapa completo do teclado HP48](teclado-hp48.csv)
6. [Inspeção de memória e estimativa de fotos da ESP32-CAM](MEMORIA-E-FOTOS-2026-11-09.md)
7. [Firmware-base ESP32-CAM com fotos e OTA](../../firmware/esp32cam_integrada/LEIA-ME.md)
8. [Teclado original de 19 trilhas](TECLADO-19.md)
9. [Mapa de 49 teclas da Lyke extraído da flash](teclado-lyke.csv)
10. [Operação e compilação dos firmwares Lyke](../../firmware/lyke_stm32/LEIA-ME.md)

## As duas montagens

| Montagem | Processadores | Tela / teclado | Situação |
|---|---|---|---|
| Calculadora na carcaça HP48 | GOOUUU S3-CAM N16R8 + ESP32 DevKit V1 | LCM12864D-V1.0 paralela; teclado original HP48 com 46 contatos aceitos | Firmware próprio de calculadora, fotos e API; última revisão S3 preparada, envio pendente de confirmação |
| Lyke 101078 | ESP32-CAM AI Thinker, ESP32 clássico + STM32G431CBU6 | Câmera AF, GMG12864-06D Ver.2.2 e teclado original de 19 trilhas | Instalados STM32 `lyke-r12`, ESP `lyke-r9a` em app0. OTA/revisões/MAP49 conferidos; leitor gráfico aguardando teste físico; manual 3.0 |

O HTML atual usa o STM32 para teclado, LCD, cálculo offline e divisor de
bateria no PA0. A ESP32-CAM fica com câmera, Wi-Fi, API e OTA. As placas
trocam dados pela UART em GPIO13/14 do ESP. O teclado usa 17 pinos na borda
inferior do STM32 (B13/B12) e 2 na superior. O usuário confirmou que instalou
T19 no C6 após remover seu LED. O USB está reservado para DFU.
O [firmware Lyke](../../firmware/lyke_stm32/LEIA-ME.md) completou o
mapeamento guiado de 49 teclas. O mapeador de bancada foi preservado em
[`firmware/stm32_teclado`](../../firmware/stm32_teclado/LEIA-ME.md).

## Documentos técnicos já existentes

- Manual do comprador em `output/pdf/Manual-Lyke.pdf` (36 páginas),
  `Manual-Lyke.md` e `Manual-Lyke-Completo.zip`. Edição 3.0, funções R12/R9a,
  telas simuladas, Wi-Fi, fotos, arquivo e tutoriais de conta OpenAI/Claude.
  **Somente OpenAI integrada/testada; Claude ainda exige adaptador no servidor.**
  Gerador e notas de revisão: `scripts/manual_lyke/`.

- `docs/API-E-ARQUIVO.md`: API implantada, arquivo privado, modelo e testes de 23/11.

- `firmware/calculadora/LEIA-ME.md`, `IMPLEMENTACAO.md`, `PRONTO-PARA-ENVIAR.md`:
  aplicação da calculadora e estado de entrega.
- `firmware/calculadora/ATUALIZAR-POR-WIFI.md`, `ATALHOS-E-FOTOS.md`,
  `VER-FOTO-NO-WIFI.md`: operação e atualização.
- `firmware/camera_usb/LEIA-ME.md`: câmera USB/UART atual.
- `outputs/ligacoes/ESP-CAM-esquema.html`: esquema interativo offline.
- `outputs/ligacoes/stm32-esp32cam.json`: dados da pinagem atual proposta.
- `outputs/ligacoes/teclado-19.json`: pinagem histórica com PCF8575.
- `outputs/ligacoes/ESQUEMA-GERAL-ESP-CAM-PCF8575.{md,json,svg,png}`:
  esquema histórico 4×5.
- `outputs/ligacoes/PCF8575-no-modulo-4067.{md,json}`: conversão dos terminais.
- `LEIA-ME-MIGRACAO.md`: histórico inicial da S3; partes intituladas “atual”
  ficaram antigas. Não usar como estado atual da ESP32-CAM clássica.

## Texto para iniciar outro chat

> Continue o projeto em C:\Projects\CAMERA_ESP. Leia AGENTS.md e
> docs/conhecimento/README.md, ESTADO-E-RETOMADA.md, HARDWARE.md e DECISOES.md.
> A Lyke usa STM32 lyke-r12 e ESP32 clássico lyke-r9a, ambos gravados.
> ESP R10 está preparada, não gravada: usuário adiou por bateria crítica.
> API v4 publicada; leia a revisão de energia e respostas antes de atualizar.
> Loader V2 instalado por USB com mapa/preferências preservados. OTA do STM
> pelo Wi-Fi/UART passou. Usuário confirmou raiz/frações na janela do LCD.
> Divisor corrigido para dois 470 kilohms; ícone e leitura da bateria confirmados.
> Manual final edição 2.0, repositório público e projeto encerrado pelo usuário.
> As 49 teclas estão mapeadas. O ESP abre Calc-Lyke automaticamente.
> UART A2→GPIO13, A3←GPIO14 e GND comum confirmada com 49 teclas.
> API e arquivo privado configurados; veja docs/API-E-ARQUIVO.md.
> Separe a calculadora S3 + DevKit da bancada ESP32-CAM clássica. Confira o
> hardware conectado antes de gravar. O esquema mais recente está em
> outputs/ligacoes/ESP-CAM-esquema.html. Depois continue a tarefa que eu indicar.

## Como manter esta base

Fontes e medições atuais prevalecem sobre relatos históricos. Registrar data,
alvo, versão, hash e resultado de cada gravação/teste; não concluir sucesso
apenas porque compilou. `inventario.json` mantém o retrato histórico de 09/11
e registra o conjunto Lyke verificado em 22/11 na seção `lyke_current`.
Conferir os manifestos atuais antes de gravar.
Não copiar senhas ou chaves para novos relatórios públicos.
