# Base de conhecimento — CAMERA_ESP

Atualização: **24/11/2026**, America/Sao_Paulo. Raiz atual:
`C:\Projects\CAMERA_ESP`. Esta é a memória local do projeto para continuar em
outro chat; não é uma garantia de memória automática fora desta pasta.

## Começar por aqui

**Último pedido:** preparados STM R12 + ESP R9, ainda não instalados,
com novo aviso Wi-Fi e desenhos roláveis. API v3 publicada e compatível
com texto antigo. O usuário ligará depois para atualizar. Guia em
`docs/REVISAO-R12-DESENHOS-E-WIFI.md`; ver o início de `ESTADO-E-RETOMADA.md`.

**Instalados:** STM32 R11 + ESP Lyke R8, atualizados pelo
Wi-Fi/UART existente. Ícone de bateria e leitura normal confirmados pelo
usuário após corrigir o divisor para dois resistores de 470 kilohms.
Notação gráfica na janela original e leitor de seis linhas mantidos.
Manual final edição 2.0 em PDF/MD em `docs/manual/` e `output/pdf/`.
Repositório agora público; credenciais, fotos de questões e binários
continuam privados. O encerramento de 23/11 é histórico; houve novo pedido em 24/11.

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
| Lyke 101078 | ESP32-CAM AI Thinker, ESP32 clássico + STM32G431CBU6 | Câmera AF, GMG12864-06D Ver.2.2 e teclado original de 19 trilhas | Instalados STM32 `lyke-r11`, ESP `lyke-r8` em app0. OTA, 49 teclas, raiz/frações e indicador de bateria confirmados; manual final R11/R8 |

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
  `Manual-Lyke.md` e `Manual-Lyke-Completo.zip`. Edição 2.0, funções R11/R8,
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
> A Lyke usa STM32 lyke-r11 e ESP32 clássico lyke-r8, ambos gravados.
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
