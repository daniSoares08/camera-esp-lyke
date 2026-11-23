# Lyke R10/R10a — instalação e atualização pelo Wi-Fi

23/11/2026 · STM32G431CBU6 + ESP32-CAM AI Thinker, ESP32 clássico, 4 MiB.

**Instalados em 23/11/2026: STM32 R10a + ESP Lyke R8.** A base R10 foi
gravada pelo USB com backup e releitura, preservando mapa/preferências.
O usuário confirmou menu e teclas. Depois, R10a foi instalada em 12,44 s
pelo Wi-Fi do ESP e UART existente, sem usar USB para essa transferência.
O portal confirmou reinício, revisão R10a e 49 teclas mapeadas.
R10a mantém as funções da R10; seu identificador diferente comprova a troca
efetiva de aplicação. O usuário confirmou no aparelho que raiz e soma de
frações cabem inteiras na mesma janela e funcionam.

## O que muda

- Atualização do STM pelo portal, usando a UART existente: STM PA2 → ESP
  GPIO13, STM PA3 ← ESP GPIO14 e GND comum. Sem novos fios.
- Entrada direta no carregador residente, evitando depender do pedido
  na RAM durante reset, caminho que falhou na R8 instalada.
- Arquivo conferido integralmente no ESP antes de pedir o carregador.
  O STM recebe em área separada, verifica e só então substitui o programa.
- Progresso de envio no portal, seguido da indicação de instalação.
  Fotos temporárias só são apagadas se faltar memória para o pacote.
  Fotos já arquivadas na nuvem não são apagadas por essa operação.
- Frações empilhadas, inclusive resultado; raiz com barra sobre a expressão,
  raiz cúbica/n-ésima, potências elevadas e símbolos π, Δ, θ e Ω.
  Expressões muito altas ou sem layout implementado usam o formato linear.
  Cursor e rolagem horizontal acompanham a edição.
- Cabeçalho normal preservado; respostas da IA ocupam as seis linhas somente
  com conteúdo. Inclui o diagnóstico de ADC/bateria preparado na R9.

### Teclas para conferir

| Ação | Teclas |
|---|---|
| Fração | `a b/c`, mantendo a sequência atual de numerador/denominador |
| Alternar resultado decimal/fracionário | `a b/c` depois de `=` |
| Raiz / potência | Teclas atuais `√`, `x²`, `^`; funções SHIFT preservadas |
| π | `SHIFT`, `EXP` |
| Variável Δ | `ALPHA`, `^` |
| Variável θ | `ALPHA`, `log` |
| Variável Ω | `ALPHA`, `EXP` |
| Guardar/recuperar variável grega | `SHIFT`, `RCL` / `RCL`, depois `^`, `log` ou `EXP` |

Δ, θ e Ω são variáveis numéricas separadas, inicialmente zero, como as
outras memórias. Os símbolos não são unidades automáticas.

![Prévias do código do LCD; simulações, sem teste físico](../../output/previews/lyke-r10-telas.png)

## Primeira instalação: USB uma vez

**Instalar carregador e aplicação juntos. Não instalar só `main.bin`, nem
enviar a R10 ao carregador antigo.** O novo formato e o novo mapa exigem a
primeira instalação conjunta. ESP R8 bloqueia o carregador antigo antes de FLASH.

1. Quando o usuário conectar o STM, reidentificar o dispositivo. Conferir
   STM32G431CBU6, DFU `0483:df11`, interface de flash interna de 128 KiB e
   serial observado agora. Se necessário, usar BOOT0/NRST para entrar no DFU
   de fábrica. O serial histórico não substitui a identificação atual.
2. Conferir o pacote sem acessar dispositivo:

   ```powershell
   .venv\Scripts\python.exe firmware/lyke_stm32/bootstrap_usb.py
   ```

3. Só na sessão com hardware identificado, executar:

   ```powershell
   .venv\Scripts\python.exe firmware/lyke_stm32/bootstrap_usb.py --install --serial SERIAL_OBSERVADO --tool "C:\Users\campo\AppData\Local\Arduino15\packages\arduino\tools\dfu-util\0.11.0-arduino5\dfu-util.exe"
   ```

   O utilitário salva os 128 KiB originais em `.private/stm32_lyke_r10_.../`,
   grava somente até `0x0801e7ff`, relê a flash e compara exatamente o mapa
   do teclado/preferências com o backup. Não usa mass erase nem option bytes.
   Sem `--install`, não abre USB/serial nem altera o aparelho.
4. Soltar BOOT0, reiniciar e conferir mapa, MODE, LCD, preferências e revisão
   `STM32 lyke-r10a`. Não remapear por rotina.
5. Pelo portal atual, instalar somente a aplicação ESP Lyke R8 fornecida
   em `ESP32/esp32cam_integrada.ino.bin`. Não gravar tabela/NVS/bootloader do ESP.
   Confirmar R8 e retorno de `Calc-Lyke` após reiniciar.
6. Fazer uma atualização real do STM pelo portal. Essa verificação confirmará
   o objetivo no aparelho montado.

## Atualizações seguintes pelo Wi-Fi

1. Ligar, conectar a `Calc-Lyke` e abrir `http://192.168.8.1`.
   Atualizar não precisa de internet.
2. Em **Atualizar firmware → STM32**, escolher **`lyke-stm32-ota.bin`**
   do pacote compatível. O `.bin` USB e o `.bin` do ESP têm destinos diferentes.
3. Pressionar **Atualizar STM32**. A barra mede envio ao ESP; em 100% ainda
   há verificação e instalação. Manter alimentação até a confirmação.
4. Aguardar **STM32 atualizado e reiniciado** e conferir revisão e teclado.
   A operação normal prevista dispensa USB, BOOT0 e NRST. Em caso de erro,
   guardar a mensagem e conferir a versão antes de reenviar.

O AP continua sem expiração. **MODE → Desligar Wi-Fi** vale apenas até
reiniciar o ESP. A câmera é parada durante a atualização.

## Memória e formato

| Área STM32 | Endereço | Tamanho |
|---|---|---|
| Carregador V2 | `0x08000000` | 8 KiB |
| Aplicação | `0x08002000` | 64 KiB |
| Pacote temporário compactado | `0x08012000` | 48 KiB |
| Metadados | `0x0801e000` | 2 KiB |
| Mapa do teclado | `0x0801e800` | 2 KiB |
| Preferências/programa | `0x0801f000`, `0x0801f800` | 4 KiB |

`LYKEG432`: cabeçalho de 24 bytes com tamanhos e dois CRC32, seguido de
blocos DEFLATE independentes de até 4096 bytes descompactados. O decoder
compartilhado confere limites, vetores, tamanho e CRC da aplicação.
`LYKE BOOT V2` identifica o carregador do novo mapa.

O carregador valida antes de apagar o programa e grava os vetores por último.
Se a cópia for interrompida, o staging válido permite repeti-la no próximo
boot. Transferência incompleta antes do compromisso mantém a aplicação anterior.
Essas propriedades passaram em testes nativos; quedas reais ainda não foram ensaiadas.

Não misturar imagens antigas `LYKEG431` com este conjunto. Manter cópias
antigas para recuperação, sem usá-las como atualização normal da base V2.
O pacote USB tem 124.928 bytes, limpando staging/metadados antigos e
terminando antes dos últimos 6 KiB de configuração.

## Compilar e verificar

```powershell
wsl --cd /mnt/c/Projects/CAMERA_ESP sh firmware/lyke_stm32/build.sh lyke-r10a
.venv\Scripts\python.exe firmware/esp32cam_integrada/build.py --profile lyke --revision lyke-r8
.venv\Scripts\python.exe firmware/lyke_stm32/tests/run_native.py
.venv\Scripts\python.exe scripts/prepare_lyke_r10.py
```

Manifestos: `.build/lyke_stm32/lyke-r10a/manifest.json` e
`firmware/esp32cam_integrada/bin/lyke-r8/manifest.json`.
Relatório nativo: `.build/lyke_stm32/native-tests.json`.
Pacote local organizado: `output/firmware/lyke-r10-pronto/`.
Os binários são privados; não publicar. Licença do decoder em `vendor/puff.h`.

### Verificações concluídas

Compilação ARM e Arduino ESP32 concluída. Testes usam os códigos reais do
renderizador, interface, carregador e transporte, com hardware simulado nas bordas:

- Teclado, fração e resultado, raízes, potências, precedência, símbolos,
  memória, cursor/rolagem e seis linhas de resposta sem cabeçalho.
- 514 interrupções num exemplo de 4096 bytes e 203 pontos amostrados na
  aplicação completa: retomada exata, carregador/teclado/preferências preservados.
  Interrupções antes das operações; não simula escrita elétrica parcial.
- Pacotes corrompidos/incompletos, vetores inválidos, limites do decoder e
  250 mutações com CRC externo recalculado.
- Transporte: envio completo, CRC recusado antes do boot, falta de ACK,
  aplicativo/carregador antigos, autorização e memória insuficiente.
- Guardas do pacote USB e recusa de releitura com configuração alterada.
  Regressões de bateria e paginação aprovadas.

**Verificação física concluída:** base USB com identificação/backup/releitura,
mapa/preferências preservados, menu/teclas confirmados pelo usuário e OTA
real R10→R10a pelo ESP. Relatório técnico em `docs/VALIDACAO-R10-2026-11-23.md`.
O usuário também confirmou raiz/frações completas dentro da moldura.
**Pendentes:** arranque frio e conferência das fotos após a atualização.
Bateria com multímetro e revisão do manual PDF/MD ficam para depois.
Sem chamada paga à API nesta etapa.
