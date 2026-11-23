# Teste independente da câmera

O mesmo fonte suporta duas placas, com seleção pelo processador na compilação:

- GOOUUU ESP32-S3-CAM N16R8: pinagem original, USB nativa CDC.
- ESP32-CAM AI Thinker/clones: ESP32 clássico, flash de 4 MiB, pinagem do
  CameraWebServer oficial e transporte UART a 460800 baud pelo conversor USB.

A placa conectada em 07/11/2026 foi identificada na COM17 como ESP32-D0WD-V3,
revisão 3.1, flash de 4 MiB. Não é um S3. O firmware da calculadora não é usado
neste teste. Não inicializa Wi-Fi, não acessa a API e não usa créditos.

Para compilar e gravar a placa nova:

```powershell
& '.irmware\Compilar firmware.ps1' -Placa ESP32CAM
& '.irmware\Gravar firmware.ps1' -Placa ESP32CAM -Porta COM17
```

Os arquivos próprios dessa placa ficam em firmware/release_esp32cam, separados
os binários S3 em release_pc. O gravador ESP32CAM valida hashes e seleciona
explicitamente --chip esp32, bootloader em 0x1000 e flash de 4 MiB.

Abra Abrir camera.bat na raiz. Escolha a porta correta e conecte. Também é
possível iniciar controle_camera.py --port COM17 --connect pelo Python local.
O painel usa 115200 na USB nativa e 460800 em conversores USB/UART.

O sensor OV5640 é detectado pelo PID. O firmware carrega o programa de autofoco,
permite foco único, contínuo e pausa. O controle manual não validado fica
desativado e seu comando é recusado sem escrever nos registradores.
Focar agora consulta também o resultado da área central: o estado 0x10,
sozinho, significa ciclo terminado. Confira a nitidez em Ver em 100%.

Tirar foto / Espaço salva o JPEG original e medições em outputs/fotos. O painel
mostra prévia automaticamente após conectar. Ajustes só persistem ao selecionar
Salvar ajustes como padrão na câmera. A câmera fica inicializada durante este
teste de bancada para a prévia e os controles; este é um firmware diferente da
calculadora, que usa standby entre capturas.

Pinagem AI Thinker: D0..D7 = 5,18,19,21,36,39,34,35; XCLK=0, PCLK=22,
VSYNC=25, HREF=23, SDA=26, SCL=27, PWDN=32, RESET=-1. Flash LED GPIO4 apagado.
Referência: https://github.com/espressif/arduino-esp32/blob/3.3.6/libraries/ESP32/examples/Camera/CameraWebServer/camera_pins.h

## Prévia e foco — revisão de 08/11/2026

A prévia gira a imagem e reduz para caber no painel preservando a proporção;
não recorta nem estica o JPEG. Os modos listados usam 4:3 e ficam 3:4 com giro
de 90/270 graus. Para incluir a folha inteira, afaste ou reposicione a câmera.

Foi observado ACK=1 e AF=0x20 persistentes. Reiniciar liberou o foco, mas não
foi confirmada a ação que originalmente causou o estado. A revisão faz
recuperação limitada do controlador de foco, distingue erro de comunicação
de uma busca sem foco e mantém a mensagem de erro durante a prévia.

Testes de contrato: outputs/tests/test_camera_focus.py. Registros de bancada:
outputs/teste_esp32cam_2026-11-08/verificacao_af_r6.json. Dois focos sucessivos
em QSXGA e um em VGA retornaram zona central zero e ACK zero; um JPEG foi
decodificado, e o comando manual foi recusado sem prender a confirmação.

Protocolo do fabricante: https://usermanual.wiki/Document/OV5640FirmwareUserGuideV10.952852672.pdf
