# Estado da entrega — 03/11/2026

A revisão final está compilada e pronta para envio por Wi-Fi quando a bateria
carregar. Os arquivos e os próximos passos estão em PRONTO-PARA-ENVIAR.md.
Não houve tentativa de enviar firmware depois de o usuário informar bateria vazia.

## Funcionalidades

- Calculadora comum como padrão e RPN opcional; E abre menu imediatamente.
- ENTER confirma/calcula sem segurar. SPC captura uma foto local, até três.
- Cabeçalho com três quadradinhos; o da captura pisca e preenche ao concluir.
  Ícone mínimo de Wi-Fi somente conectado; removido ALG/DEG/OFF.
- Exclusão de funções como tan( em um gesto; constantes simbólicas e glifo π.
- Um, dois ou três JPEGs enviados juntos no mesmo pedido; base64 em fluxo evita
  cópias expandidas de todo o conjunto. Fotos preservadas se houver erro.
- Galeria /foto com Foto 1/2/3, download original e estado da câmera.
  Abrir/visualizar a página não captura nem envia consulta à API.
- IPHONEPROMAX provisionado/migrado mantendo rede de casa e chave da API.
- Busca Wi-Fi assíncrona com espera por conclusão e erro explícito.
- Revisão final: encerra tentativa anterior antes de conectar e registra motivo
  de desconexão do driver. Falhas não mantêm tentativas automáticas indefinidas.
- Senha local fixa CHANGE_ME_ESP_AP; OTA dos dois módulos sem prazo de dez minutos.
  O portal de fotos/configuração conserva dez minutos por inatividade.

## O que já foi verificado

A versão de galeria anterior à revisão final foi instalada no S3 por OTA,
com reinício em app0 confirmado. Três capturas físicas por SPC foram baixadas
separadamente: 373956, 578276 e 468886 bytes, todas 2560×1920 e com hashes
individuais diferentes. Slot inexistente foi recusado. Sensor em standby,
interface da câmera inativa e nenhuma tentativa de API nas capturas.

A busca real encontrou nove redes, incluindo IPHONEPROMAX. O usuário confirmou
por captura de tela o nome/senha do hotspot, acesso permitido e compatibilidade
ativada. A conexão ainda não foi confirmada; a bateria acabou antes de concluir
esse teste. Não atribuir a falha a espaços no nome sem evidência.

O DevKit aceitou seu binário por OTA com HTTP 200. Falta a conferência do slot
após reiniciar. A revisão final do S3 (diagnóstico de conexão e ajustes da página)
ainda não foi enviada e não possui validação no aparelho.

Testes locais passaram: matemática/domínios/RPN, E/SPC, exclusão por função,
glifo π, indicadores, JSON/base64 real para 1/2/3 imagens, CRC/protocolo,
preservação do mapa/partições e galeria JavaScript com seleção/download,
metadados atuais, standby e ações explícitas de captura/consulta.

Nenhuma consulta paga foi feita nesta etapa. Resposta real da IA com várias
imagens e medição de corrente permanecem pendentes.

## Hardware preservado

S3 GOOUUU N16R8, PSRAM OPI 8 MiB, flash 16 MiB; câmera OV5640 de foco fixo.
DevKit ESP32-WROOM, flash 4 MiB. S3 TX43 → DevKit RX16 e GND comum, sem retorno.
LCD DB0..7: 4,14,13,18,19,21,22,23; CS1=25, CS2=26 ativos LOW;
RST=27, RS=32, E=33, RW=GND. Quadros e mensagens girados 180° no DevKit.
Nenhum GPIO alterado nesta revisão; OTG 19/20 continuam no teclado.

Câmera inicializada sob demanda. Copia JPEG para PSRAM, solicita standby
registrador 0x3008, confirma quando possível, encerra driver e XCLK antes de
HTTP/API. Não há GPIO para corte físico de VDD. O ruído relatado ao abrir
Wi-Fi não permite identificar sua origem; o Wi-Fi permanece ativo no portal.

S3 com dois slots OTA de 3 MiB; DevKit com dois de 1,5 MiB. NVS e sistema de
arquivos preservados. Senha/API não são retornadas pelo diagnóstico.
Internet do computador pelo cabo; comunicação com a calculadora pelo Wi-Fi.

## Registros

- outputs/calculadora/revisao_2026-11-03.json: entrega, testes e pendências.
- outputs/calculadora/revisao_2026-11-03_galeria_scan.json: metadados reais.
- outputs/calculadora/fotos/revisao_2026-11-03/: três JPEGs conferidos.
- bin/*/manifest.json: tamanhos e hashes dos binários atuais, privados.
