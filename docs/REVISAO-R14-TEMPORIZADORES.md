# STM R14 + ESP R12: sono e limite de tentativas Wi-Fi

Atualizado em 26/11/2026; preparação em 25/11. Alvo: Lyke, STM32G431CBU6 + ESP32-CAM clássica AI Thinker,
4 MiB de flash/PSRAM. Sem novos fios, troca de pinos ou alteração do loader V2.

## Estado final em 26/11/2026

**Instalados STM R14 + ESP R12 finais**, pelo Wi-Fi/UART existente. A dupla
instrumentada foi removida: primeiro STM para retirar o prazo temporário de
um minuto, depois ESP. Manifestos, digest ESP em execução, revisões, MAP49,
prontidão, valor padrão 60 min e última resposta idêntica foram conferidos.
O endpoint de diagnóstico não existe na versão final. Configurações de câmera,
serviço e redes preservadas, sem novas ligações e sem chamada paga à API.

**Teste físico aprovado pelo usuário:** SHIFT+AC apagou a tela; uma tecla
numérica não despertou; somente MODE despertou e o menu voltou a funcionar.
O portal confirmou ESP com reset de deep-sleep e despertar ext0
(`reset_reason=8`, `wake_reason=2`), STM R14/MAP49 e resposta preservada.

O ensaio real de 20 minutos desconectado foi iniciado e **cancelado a pedido
explícito do usuário**, que considerou SHIFT+AC/MODE suficiente. Não registrar
esse ensaio como aprovado. O limite, a conexão mantida, a perda de link e o
retorno manual foram validados nos testes nativos. A rede externa original foi
restaurada e confirmada conectada, com tentativas ativas e contador desconectado
zerado. O sono automático usa a mesma rotina já testada por SHIFT+AC; contagem,
configuração e persistência aprovadas localmente, sem esperar 60 min no aparelho.

Relatórios privados finais: `.private/lyke-final-2026-11-26/` e provas em
`.private/lyke-cloud/device-stm-r14-wifi.json` / `device-r12-ota.json`.
Manual final edição 4.0 em PDF/MD, com 36 páginas, em `docs/manual/` e `output/pdf/`.
Os binários de produção ficam apenas no pacote privado
`output/firmware/lyke-r14-esp-r12-final.zip`. Não publicar fotos ou binários.

## Histórico do ensaio de 25/11 (recuperado em 26/11)

O usuário informou alimentação estável e autorizou gravar, revogando a espera
anterior por carga. Naquela etapa, R14/R12 estavam compiladas,
mas a instalação final ainda estava pendente.

Foram instaladas pelo Wi-Fi as versões temporárias **STM R14-test / ESP R12-test**.
Revisões, 49 teclas, configurações e resposta salva conferidas. Uma captura
JPEG 2560x1920, 102.640 bytes, com pausa/restauração do rádio passou sem reinício.
Não foi feita chamada paga à API. A foto do ensaio é privada.

O teste de sono registrou ESP em deep-sleep e retorno ext0 pelo GPIO13
(`reset_reason=8`, `wake_reason=2`), mas **não confirmou o ciclo STOP1 do STM**.
O monitor continuou vendo tempo de sono de um minuto, em vez do marcador de
retorno do teste. Depois o portal deixou de responder; não concluir se o
aparelho está em sono, se o portal fechou ou se houve queda de alimentação.

Na instrumentação inicial, o GPIO14/TX foi trocado para RTC antes de transmitir
SLEEPING, interferindo no aviso ao STM. O fonte de teste foi corrigido para
transmitir/flush/end antes de mudar esse pino. Também mantém o portal aberto
enquanto a versão de teste está acordada. **Essas correções de teste ainda não
foram gravadas.** A versão final não contém timer de retorno nem endpoint de teste.

**Recuperação então necessária (já concluída em 26/11):** desligar e ligar pela chave física e abrir
MODE > ATUALIZAR FIRMWARE. Não depender apenas de MODE para sair da versão
instrumentada, cujo pino de despertar do STM é temporariamente PA3.
Com a dupla de teste daquela etapa, era necessário instalar **STM R14 final primeiro**, removendo o
sono de um minuto, e depois ESP R12 final. Na migração de revisões antigas,
que não suportam S1, a ordem normal continua ESP primeiro, depois STM.

Relatórios locais: `.private/lyke-auto-2026-11-25/`. Não publicar os JSONs
com configuração nem a foto. As provas finais só podem ser criadas após
conferir a revisão, hashes e comportamento realmente instalados.

## Sono por inatividade

- Padrão **60 minutos**. Menu **MODE > MODO CIENTIFICO > SONO**.
- Faixa **1 a 1440 minutos**, gravada nas preferências existentes.
- Cima/baixo seleciona SONO; números substituem/compõem o valor;
  esquerda/direita ajusta um minuto, DEL apaga o último dígito, `=` salva/sai.
  AC e MODE também salvam ao sair das preferências.
- O relógio de inatividade só renova com contato físico detectável no teclado,
  incluindo tecla mantida pressionada ou múltiplos contatos. ON não está ligada
  ao mapa de 49 teclas e não pode ser detectada.
- UART, STATUS, página HTTP aberta e leitura remota não contam como botão.
- Captura, envio, leitura de resposta em trânsito, busca/listagem e abertura
  de portal pendentes adiam a tentativa; atualização ativa é recusada pelo ESP.
  Uma recusa não zera a inatividade; a nova tentativa aguarda 60 s.
- SHIFT+AC usa a mesma coordenação de sono; MODE é o único despertar final.
  ESP deep-sleep/ext0 GPIO13 e STM STOP1, com SysTick/ADC/UART parados.
  O protocolo e a pinagem estão em [sono profundo](REVISAO-R13-SONO-PROFUNDO.md).
- A entrada é limpa. Demais memórias do STM ficam em RAM. Fotos temporárias
  no ESP se perdem; última resposta/desenho gravados, configurações e mapa ficam.

O valor usa os bits 9 a 19 do campo já existente de preferências. O registro,
tamanho e checksum não mudam. Preferências antigas, sem valor, usam 60 minutos.

## Vinte minutos continuamente sem Wi-Fi

O driver Arduino tinha autorreconexão habilitada; ele pode repetir tentativas
conforme o motivo da desconexão. O aplicativo já encerrava uma tentativa de
associação após seu próprio timeout. A R12 acrescenta um limite explícito de
**1.200.000 ms continuamente sem conexão**, independentemente dessas condições.

Conectado, o prazo não atua. Após perder uma conexão, começa uma janela nova.
No vencimento, o ESP encerra busca/associação, desabilita autorreconexão e
desliga STA. Se Calc-Lyke estiver aberta, mantém apenas o AP até seu prazo de
inatividade; sem portal, rádio OFF. Captura/OTA/reboot adiam o desligamento
até ser seguro. Uma conexão recém-estabelecida é conferida novamente antes
de parar o rádio.

WIFI SALVO, PROCURAR WIFI, CONFIG. CELULAR, Conectar no portal ou novo boot
renovam a janela. Teclas comuns de cálculo não a renovam. O limite não é
configurável. Depois de pausado, o aviso de erro Wi-Fi também para de piscar.
A restauração da câmera não reativa tentativas já pausadas.

O portal normal abre no boot/menu e fecha após 120 s sem HTTP. Uma página
aberta consulta o aparelho e mantém o portal ativo; feche-a depois de configurar.
A câmera pausa o rádio automaticamente e o restaura após PWDN, sem reduzir
resolução, JPEG ou autofoco. CPU 80 MHz no repouso, 160 MHz em captura/HTTPS.

## Compilação e verificações

- STM R14: aplicação **65.180/65.536 bytes**, CRC `3fc297ea`; OTA
  **47.733/49.152 bytes**. Loader idêntico à R12; mapa e preferências preservados.
- ESP R12: **1.379.440/1.572.864 bytes**, manifesto local com
  `sleep_selftest=false`. Binários privados em `output/firmware/`.
- **32 verificações nativas aprovadas**: teclado/menu/valor salvo,
  compatibilidade de preferências, 60 min/limites, tecla mantida, overflow do
  relógio, adiamento/repetição, Wi-Fi conectado por 25 h simuladas, perda de
  link, 20 min desconectado, captura/OTA ocupadas e regressões anteriores.
- Real no aparelho: OTA da dupla de teste, resposta preservada e JPEG sem reset.
- Em 26/11: instalação final e sono/retorno por MODE confirmados. Ensaio real
  de 20 min dispensado pelo usuário, sem alegação de aprovação. Consumo,
  temperatura e autonomia não medidos; resistores/conversor seguem alimentados.

## Manual e publicação

Edição final **4.0, 26/11/2026**, 36 páginas: PDF, Markdown integral, imagens e
ZIP portátil em `output/pdf/`; PDF/MD/imagens sincronizados com `docs/manual/`.
Prévia de 25/11 preservada em `output/pdf-previa-r14/`, identificada como histórica.
Fonte do manual usa frames do renderizador R14; sono, limite Wi-Fi, energia,
configuração e validação foram atualizados. Apenas OpenAI integrada/testada.

O snapshot do GitHub contém somente fontes, esquema e documentação saneados.
Credenciais, fotos de questões, NVS, backups e binários ficam privados.
O modo `--sleep-selftest` exige revisão terminada em `-test`; seus temporizadores,
pino alternativo e endpoint são excluídos de produção. O empacotador recusa
revisões de teste e exige provas de instalação para marcar um pacote como final.
