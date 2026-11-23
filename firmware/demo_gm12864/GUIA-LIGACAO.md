# Teste da telinha GM12864-59N com ESP32-S3

Firmware preparado para a placa ESP32-S3 N16R8 das suas fotos e a tela marcada **GM12864-59N VER:2.0 IIC**. Essa tela é um LCD gráfico de **128 × 64 pixels**, com controlador **ST7567S** e comunicação **I²C**. O modelo e a interface foram conferidos na documentação do produto e no projeto OpenDTU, que usa o mesmo driver U8g2.

## 1. Ligue estes quatro fios

**Retire o cabo USB e desligue a alimentação antes de ligar os fios.** Use quatro jumpers e siga as letras impressas nas placas.

| Pino na tela | Pino na ESP32-S3 | Função | Cor sugerida |
|---|---|---|---|
| **VCC** | **3V3** | Alimentação de 3,3 V | Vermelho |
| **GND** | **GND** | Terra comum | Preto |
| **SCL** | **42** (GPIO42) | Clock I²C | Amarelo |
| **SDA** | **41** (GPIO41) | Dados I²C | Azul |

Na **Foto 4**, olhando o verso da tela com o conector no alto e a escrita do modelo legível, a sequência da esquerda para a direita é **VCC — GND — SCL — SDA**.

Na **Foto 2**, com as duas USB à esquerda e a antena à direita:

- **3V3:** último furo à direita da fileira de cima, ao lado de EN.
- **GND:** primeiro furo à esquerda da fileira de baixo, próximo às USB.
- **41 e 42:** fileira de baixo, perto da antena. O trecho impresso é `39 — 40 — 41 — 42 — 2 — 1 — RX — TX`.

Os números **41 e 42 são os nomes dos GPIOs**, não a contagem dos furos. Na sua foto, a ESP ainda está sem barras de pinos: solde os conectores ou os fios nesses furos para ter contato firme. Só encostar um jumper no furo pode causar falhas intermitentes.

```text
TELA                       ESP32-S3
VCC ---------------------- 3V3
GND ---------------------- GND
SCL ---------------------- 42
SDA ---------------------- 41
```

Alimente a ESP por **uma USB-C com cabo de dados**; ela fornece os 3,3 V à tela. Embora a placa da tela diga 3,3–5 V, use **3V3 nesta montagem** para manter os sinais I²C compatíveis com a ESP32. Não ligue VCC da tela ao 5V nesta ligação direta. Os GPIO35/36/37 são usados pela memória nas variantes com PSRAM octal; o firmware usa 41/42. Não são necessários CS, RESET externo nem ligação ao conector flat da tela.

## 2. O que aparece

A demo começa no robozinho e troca de cena automaticamente a cada dez segundos:

1. **Robozinho:** olhos piscam e acompanham o movimento; braço acena.
2. **Radar:** varredura circular com alvos fictícios; a tela indica SIMULACAO.
3. **Cubo 3D:** cubo girando com projeção em perspectiva.
4. **Teste de pixels:** bordas e diagonais, xadrez, linhas, todos os pixels ligados e todos desligados. Cada padrão dura dois segundos; tela totalmente apagada nessa etapa é esperado.

**BOOT, depois de iniciar:** toque curto troca a cena; segure por aproximadamente um segundo para pausar/continuar a animação e a troca automática. Não segure BOOT durante uma reinicialização para usar a demo, pois isso entra no modo de gravação.

O firmware funciona sem computador depois de gravado, usando uma fonte USB. A demo não inicializa câmera, Wi-Fi ou cartão SD. Os ajustes de contraste/orientação são temporários e voltam ao padrão ao reiniciar. Ao gravar esta demo, o programa atual da ESP é substituído; os fontes e binários da câmera continuam nas outras pastas deste projeto.

## 3. Gravar os binários prontos neste computador

Os arquivos compilados ficam em **bin** nesta pasta. Não é preciso recompilar para usar o pacote pronto na placa N16R8.

1. Feche o painel da câmera e qualquer monitor serial.
2. Conecte somente a **USB-OTG**, identificada no verso da ESP (Foto 3), com cabo de dados.
3. Segure **BOOT**, pressione e solte **EN/RST**, e depois solte **BOOT**.
4. No Gerenciador de Dispositivos, veja a porta **COM** que apareceu no modo Download. Ela pode ser diferente da porta usada pelo programa normal.
5. Abra o PowerShell na pasta `C:\Projects\CAMERA_ESP` e execute, trocando COM7 pela porta encontrada:

```powershell
powershell -ExecutionPolicy Bypass -File ".\firmware\demo_gm12864\Gravar.ps1" -Porta COM7
```

O script usa o Python e esptool já instalados em `.venv`, verifica os hashes e grava bootloader, tabela de partições, seleção de boot e aplicação. Ele não executa uma limpeza completa da flash. Se o programa não iniciar depois da gravação, solte BOOT e aperte EN/RST.

**A criação deste pacote não grava a placa automaticamente.** A confirmação física será ver o robozinho e os outros desenhos depois que você ligar e gravar.

## 4. Gravar pela Arduino IDE

Em outro computador, a opção mais simples é abrir **demo_gm12864.ino** na Arduino IDE:

1. Instale o pacote **esp32 by Espressif Systems**. Este firmware foi compilado com a versão **3.3.6**.
2. No gerenciador de bibliotecas, instale **U8g2 by oliver**. A versão usada na compilação é **2.36.19**.
3. Selecione a placa **ESP32S3 Dev Module** e configure:

| Opção | Valor |
|---|---|
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| USB Mode | USB-OTG (TinyUSB) |
| USB CDC On Boot | Enabled |
| Partition Scheme | Default |
| Upload Speed | 460800, ou 115200 se houver falha |

4. Use a USB-OTG, entre em Download com BOOT + EN/RST, selecione a porta que apareceu e clique em **Upload**.
5. Depois, pressione EN/RST com BOOT solto. Se quiser usar o monitor serial, selecione novamente a porta USB após a reinicialização.

## 5. Controles e diagnóstico

Abra o monitor serial da **USB-OTG** a **115200**. Envie os caracteres abaixo; qualquer configuração de fim de linha serve.

| Comando | Resultado |
|---|---|
| `n` | Próxima cena |
| `1`, `2`, `3`, `4` | Fixa a cena escolhida e desliga a troca automática |
| `a` | Liga/desliga a troca automática |
| `p` | Pausa/continua a animação |
| `+` / `-` | Aumenta/diminui o contraste em passos de 4 |
| `r` | Gira a imagem 180° |
| `i` | Inverte claro/escuro |
| `s` | Mostra os endereços que responderam no barramento I²C |
| `h` | Mostra os comandos |

O endereço esperado é **0x3F** (7 bits); o firmware também tenta **0x3E**. O driver usa o valor deslocado em um bit, como exige a U8g2. Uma resposta I²C confirma comunicação, mas a imagem precisa ser conferida na tela para validar contraste, orientação e todos os pixels.

Se não aparecer desenho:

- **O monitor mostra `INFO` com firmware da câmera:** o programa da tela ainda não foi gravado. Compilar gera o arquivo, mas é necessário executar a gravação/upload. Ao enviar `h`, a demo correta responde com `DEMO GM12864-59N / ST7567S 128x64`.
- **Sem resposta em 0x3F/0x3E:** confira terra comum, 3V3, soldas e se SDA/SCL foram invertidos. O firmware tenta reconectar a cada três segundos. Desligue antes de corrigir os fios.
- **Responde, mas a tela está clara ou escura:** envie vários `+` ou `-` para encontrar o contraste; padrão inicial 200 na escala da U8g2 (0–252).
- **Imagem de cabeça para baixo:** envie `r`.
- **Imagem invertida:** envie `i`.
- **Tela apagada apenas durante o teste de pixels:** espere dois segundos; esse é um dos padrões da demo.
- **Falha de upload:** use a COM do modo Download, feche programas que usam a porta, confira o cabo de dados e repita BOOT + EN/RST.
- **I²C instável com fios longos:** encurte os jumpers. Se forem necessários pull-ups externos, use 4,7 kΩ de SDA para **3V3** e outro de SCL para **3V3**; comece com o módulo como veio, que já possui resistores na placa.

## 6. Recompilar neste projeto

```powershell
powershell -ExecutionPolicy Bypass -File ".\firmware\demo_gm12864\Compilar.ps1"
```

O script compila o sketch separado, coloca os binários em `bin` e cria `manifest.json` com os hashes. Recompilar não grava a placa. Os scripts de compilação/gravação existentes da câmera continuam apontando para o firmware da câmera.

## Fontes consultadas

- [Tabela do produto GM12864-59N: ST7567S, 128×64, I²C e quatro pinos](https://haptosdisplay.com/lcd-display/).
- [OpenDTU: suporte à tela ST7567S GM12864-59N](https://www.opendtu.solar/hardware/display/).
- [OpenDTU: driver U8g2 e endereço 0x3F para este modelo](https://github.com/tbnobody/OpenDTU/blob/master/src/Display_Graphic.cpp).
- [U8g2: construtores e parâmetros de ligação](https://github.com/olikraus/u8g2/wiki/u8g2setupcpp).
- [Espressif: API I²C com pinos configuráveis](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2c.html).
- [Espressif: datasheet do ESP32-S3-WROOM-1 e restrições da PSRAM octal](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf).

As posições físicas de 3V3, GND, 41 e 42 foram lidas nas fotos fornecidas. Não se deve usar a posição de um pinout genérico de outra revisão como substituta dos rótulos desta placa.
