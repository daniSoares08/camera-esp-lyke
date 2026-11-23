# Verificação do pacote

- Compilação para ESP32-S3 concluída em 14/10/2026, usando Arduino CLI, core esp32 3.3.6 e U8g2 2.36.19.
- Configuração: flash 16 MB, modo DIO, 80 MHz, PSRAM OPI, USB-OTG/TinyUSB e CDC no boot.
- Programa: 415.507 bytes, 31% da partição de aplicação de 1.310.720 bytes.
- Variáveis globais: 46.208 bytes, 14% da RAM considerada pelo compilador.
- Aplicação verificada com `esptool 5.1.0 image-info`: chip ESP32-S3, checksum válido e hash de validação válido.
- Scripts PowerShell analisados pelo parser sem erros de sintaxe.
- Pinagem do fonte e do guia conferida: SDA=41 e SCL=42.
- Arquivos de gravação e hashes individuais registrados em `bin/manifest.json`.

Atualização do teste com a placa conectada em 14/10/2026:

- A placa inicialmente executava `usb-sd-video-20261013-r8`, confirmado pela resposta serial `INFO`; a demo ainda não estava em execução.
- A demo foi gravada por USB, com os hashes dos quatro componentes verificados pelo esptool.
- Depois da reinicialização, a porta COM10 respondeu com `DEMO GM12864-59N / ST7567S 128x64`.
- O scan I²C confirmou o LCD em **0x3F**, com SDA=41 e SCL=42.
- A cena do robozinho foi selecionada para a conferência visual, e o usuário confirmou: **"Sim, apareceu"**. A troca automática foi reativada depois dessa confirmação.
- O backup da flash foi interrompido a pedido do usuário; não foi necessário apagar toda a flash.

Registros seriais em `outputs/diagnostico_tela_branca`. Comunicação I²C e imagem do robozinho confirmadas. O teste de pixels está disponível para a inspeção detalhada de todo o painel.

Use `Gravar.ps1` com os quatro componentes separados. O arquivo `demo_gm12864.ino.merged.bin`, que o compilador também gera na pasta local, não é necessário para este procedimento e não está incluído no ZIP: ele cobre toda a flash e pode sobrescrever os dados de outras partições se gravado integralmente.
