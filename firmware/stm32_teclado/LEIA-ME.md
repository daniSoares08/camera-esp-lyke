# Mapeador de 19 trilhas STM32

Alvo STM32G431CBU6, 128 KiB de flash em `0x08000000`. O STM32 executa o teste
com alimentação pelo USB-C da própria placa. A tela GMG12864-06D recebe 3,3 V
do STM32. Teclado passivo isolado da placa original. ESP e bateria podem
ficar desconectados.

O mapa vem de `outputs/ligacoes/stm32-esp32cam.json`. T19 usa PC6 (C6) com o
LED de usuário D1 removido. O LED vermelho P sinaliza alimentação. PA11/PA12
são USB. LCD SPI1: CS=A4, RSE=A6, RS=A1, SCL=A5, SI=A7, VDD=3V3 e VSS=GND.

## Mapeamento

- A tela exibe `MAPEADOR STM32 R4`, a contagem e um relógio de atividade.
  A imagem gira 180° para a posição física instalada.
- O contraste fica no nível 5 desde a inicialização. Antes de registrar o
  primeiro par, a tela mostra `CONTRASTE 5 FIXO`.
- Solte todas as teclas após ligar. Pressione uma tecla por vez e espere
  aparecer o par `Txx - Tyy` e os GPIOs. Anote o par junto do nome físico da
  tecla. O número só incrementa para um par novo. Um par repetido exibe o
  número anterior. A contagem e o último par ficam em RAM até reiniciar.
- A varredura põe uma trilha por vez em LOW com dreno aberto. As demais ficam
  como entradas com pull-up. Uma ligação observada num sentido ou nos dois
  conta como o mesmo par. Mais de um par ao mesmo tempo pede soltar tudo.
- `Txx BAIXA EM REPOUSO` indica trilha em nível baixo com todas as saídas da
  varredura soltas. Verificar continuidade e se as pontes USB-C opcionais estão
  abertas para PA9, PA10, PB4 e PB6.

## Compilar e gravar

Use GCC ARM (testado com 13.2.1), Python 3, CMSIS Device G4 v1.2.6 em
`.tools/stm32/cmsis-device-g4` e CMSIS 5.9.0 em `.tools/stm32/CMSIS_5`.
No WSL, a partir da raiz do projeto, execute `sh firmware/stm32_teclado/compilar.sh`.
O executável e a imagem ficam em `.build/stm32_teclado/mapeador.{elf,bin}`.

Entre em DFU com BOOT0 + NRST. No Windows use WinUSB para `0483:DF11` com
dfu-util. Confira o serial USB e a descrição da flash interna com `dfu-util -l`.
Selecione a interface de flash interna e `0x08000000`, grave o `.bin` e
confira o conteúdo lendo de volta antes de reiniciar com NRST.
