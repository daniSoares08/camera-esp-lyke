# Verificação — 16/10/2026

## Atualização v4 — configuração confirmada com teste parado

- O usuário forneceu o anúncio e a folha técnica da versão WHITE: VDD de
  5 V ±0,25 V, CS1/CS2 ativos em LOW e entrada HIGH mínima de 2 V.
- O usuário confirmou VDD em 5 V, potenciômetro de 20 kΩ entre VDD/VEE,
  cursor em VO e câmera desconectada.
- As seis fotografias confirmaram cenas desenhadas e contraste variável.
  Mostravam os blocos de 64 colunas em ordem inversa: radar à direita,
  texto à esquerda e cubo dividido na borda.
- A v3 aplicou uma inversão extra (`swap=1`). Apesar da confirmação
  anterior sobre o robô, o usuário informou que ele voltou a se dividir.
  O estado serial mostrou que o ajuste não havia sido perdido: a inversão
  extra era o próprio padrão incorreto gravado na v3.
- Foi feito novo teste controlado: CS ativo LOW, `swap=0`, radar parado.
  O usuário confirmou: **"Sim, círculo à esquerda e texto à direita"**.
- A v4 inicia com o estado assim confirmado: `csLow=1 swap=0`, esquerda
  em CS1/GPIO12 e direita em CS2/GPIO13.
- Quatro componentes gravados com hash verificado pelo esptool. Aplicação
  de 418.263 bytes (31%); binário de 418.416 bytes; RAM global de 46.216 bytes.
- Após a gravação, o ESP passou por outro reinício via ROM/esptool, sem
  regravar a flash. A aplicação voltou como v4, `csLow=1 swap=0`.
  Nenhum comando de polaridade (`c`) ou troca de metades (`w`) foi enviado
  depois desse reinício. O radar foi então selecionado e pausado para
  conferência visual com a mesma referência anterior.
- O usuário confirmou após esse reinício: **"Sim, continua correto após
  reiniciar"**. O ciclo automático foi reativado com `csLow=1 swap=0`,
  sem pausa, voltando ao robô como primeira cena.
- Evidências atuais em `outputs/teste_lcm12864_v4`: `gravacao.log`,
  `reinicio.log`, `antes_reinicio.json`, `apos_reinicio.json` e referência
  visual confirmada. As fotos anteriores ficam em `outputs/teste_lcm12864_v3`.
- Contraste permanece um ajuste físico do usuário.

## Histórico da primeira gravação (v1)

- Placa identificada por USB OTG: ESP32-S3 rev. 0.2, MAC
  7C:E8:B1:A4:D7:04, flash 16 MB e PSRAM 8 MB.
- Porta do bootloader: COM9. Porta do firmware após reinício: COM10.
- Compilação: core ESP32 3.3.6, U8g2 2.36.19; 418.071 bytes de programa
  (31%) e 46.216 bytes de variáveis globais (14%). Binário: 418.224 bytes.
- `esptool image-info`: checksum e hash de validação válidos.
- Bootloader, tabela de partições, boot_app0 e aplicação gravados; os quatro
  componentes tiveram hash verificado pelo esptool. Firmware antigo substituído.
- Firmware respondeu com `build=lcm12864-demo-v1` na COM10.
- As dez cenas foram selecionadas pela USB, geraram buffers de 1024 bytes
  e continuaram atualizando os quadros com tempo de transferência de ~14 ms.
- Pausa manteve o hash da imagem estável em todas as cenas.
- Inversão alterou o buffer e restaurou o hash ao ser desfeita.
- Rotação, polaridade de CS, troca de metades e reinicialização do LCD
  responderam e foram restauradas ao padrão ao final.
- Troca automática da cena 1 para a 2 confirmada. Mais de 1.000 quadros
  enviados no teste, sem reinício observado; heap livre estável em 315.556 bytes.
- Buffers das dez cenas renderizados para inspeção de layout.
- Estado deixado ao término: cena 1, automático ligado, sem pausa, orientação
  normal, pixels normais, CS ativo alto, metades na ordem do print.

Registros: `outputs/teste_lcm12864/gravacao.log`, `serial.log`,
`verificacao.json`, `cena_01.png` até `cena_10.png` e `previa_buffers.png`.
As imagens são reconstruções dos buffers lidos do ESP, não fotografias da tela.

**Limite:** RW está fisicamente no GND; portanto não há leitura de resposta
do LCD. A execução e os buffers foram verificados, mas a imagem no painel,
a versão de tensão e o circuito de contraste ainda dependem de conferência
física. Não foi feita alteração na fiação informada.
