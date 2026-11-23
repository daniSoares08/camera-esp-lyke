# Continuidade deste projeto

Antes de trabalhar no hardware, nos firmwares ou nas ligações, leia
`docs/conhecimento/README.md` e `docs/conhecimento/ESTADO-E-RETOMADA.md`.
Para pinagens, leia também `docs/conhecimento/HARDWARE.md` e consulte os
fontes/manifestos indicados. Esta base registra a conversa até 14/11/2026.
Para o teclado da nova calculadora, consulte `docs/conhecimento/TECLADO-19.md`.

- Existem duas montagens diferentes: calculadora GOOUUU ESP32-S3 + DevKit e
  bancada ESP32-CAM clássica AI Thinker. Não misturar pinagens, imagens de
  flash, portas ou comportamento de energia da câmera.
- O nome ESP32-S3-CAM informado para a placa nova foi corrigido pela identificação
  eletrônica: a bancada recente usa ESP32 clássico, flash 4 MB, PSRAM 4 MB.
- Distinguir solicitação, ligação proposta, compilação, gravação e teste físico.
  Esquema pronto não significa circuito montado ou firmware integrado.
- Os números COM e o dispositivo conectado são históricos: identificar novamente
  antes de gravar. Conferir chip, tamanho de flash e manifesto do alvo.
- Não publicar credenciais, chaves de API ou binários privados. A base aponta
  para a configuração local sem copiar a chave de API.
- Ao mudar componentes, pinagem, firmware gravado ou pendências, atualizar esta
  base junto com os arquivos técnicos correspondentes, preservando o histórico.

As preferências do usuário estão em `docs/conhecimento/DECISOES.md`.
