# CAMERA_ESP — calculadora Lyke

Firmware, API e esquema da calculadora com **STM32G431CBU6 + ESP32-CAM clássica AI Thinker**.
[Repositório público no GitHub](https://github.com/daniSoares08/camera-esp-lyke).

Estado verificado em **23/11/2026**: STM32 `lyke-r11`, ESP `lyke-r8`, 49 teclas mapeadas.
Atualização do STM pelo Wi-Fi/UART existente testada, fórmulas gráficas na janela
126×48 e indicador de bateria confirmados no aparelho. API v2 com desenvolvimento
completo e arquivo privado no Supabase; somente OpenAI integrada/testada.

## Começar

Preparada em 24/11/2026 a [revisão R12/R9 com desenhos e novo aviso Wi-Fi](docs/REVISAO-R12-DESENHOS-E-WIFI.md),
ainda sem instalar. API v3 publicada, com suporte opcional a figuras e
compatibilidade textual. O conjunto físico continua R11/R8 até o próximo teste.

- [Estado e continuidade](docs/conhecimento/ESTADO-E-RETOMADA.md)
- [Manual ilustrado do comprador em PDF](docs/manual/Manual-Lyke.pdf)
- [Manual completo em Markdown para leitura e uso com IA](docs/manual/Manual-Lyke.md)
- [Esquema elétrico HTML offline](outputs/ligacoes/ESP-CAM-esquema.html) — baixe o arquivo e abra no navegador.
- [Hardware, alimentação e GND/R](docs/conhecimento/HARDWARE.md)
- [Firmware da Lyke e operação](firmware/lyke_stm32/LEIA-ME.md)
- [API, fotos e implantação](docs/API-E-ARQUIVO.md)
- [Mapa das 49 teclas](docs/conhecimento/teclado-lyke.csv)

## Uso

Ligue a calculadora. O ESP abre `Calc-Lyke` e tenta a última rede externa salva.
SHIFT + REPLAY direita captura uma foto; até três podem formar uma consulta.
No menu MODE, envie as fotos à IA. A resposta aparece no leitor do LCD e no portal
local `http://192.168.8.1`. O celular também pode abrir as fotos e iniciar a resolução.

O modelo principal é **GPT-6.1 Terra**, com raciocínio alto, conferência das contas,
texto ASCII e linhas de até 21 caracteres. O modo econômico usa GPT-6 Luna.
O catálogo consultado oferece Terra como 6.1, não como 6.2.

O Wi-Fi de atualização não expira. Desligar pelo STM vale até reiniciar o ESP.
O leitor dedica suas seis linhas à resposta. Fora do leitor, o cabeçalho mostra
Wi-Fi, bateria e três posições de fotos. A bateria usa quatro níveis aproximados:
`!` pisca em tensão baixa e `?` indica falha de leitura. Não é porcentagem exata.
As fotos são arquivadas quando enviadas para resolução; uma captura offline fica em RAM
e precisa ser enviada antes de desligar para entrar no arquivo da nuvem.

## Compilação

ESP no Windows: Arduino CLI, core ESP32 **3.3.6**, ArduinoJson **7.4.2**, Python e esptool.
As ferramentas locais ficam em `.tools/` e `.venv/`; elas não são publicadas neste repositório.
Crie `.private/ap-password.txt` com a senha WPA2 do portal (8–63 caracteres ASCII) e execute:

```powershell
.\.venv\Scripts\python.exe firmware/esp32cam_integrada/build.py --profile lyke --revision lyke-r9
```

STM32 no WSL, com GCC ARM e Newlib Nano:

```sh
sh firmware/lyke_stm32/build.sh lyke-r12
```

Teste da API sem consumir créditos, com Node **24**:

```sh
node --test supabase/tests/cloud.test.ts supabase/tests/graphics.test.ts
```

Os comandos compilam a revisão preparada R12/R9, ainda sem teste físico.
Os testes nativos e a geração do manual estão documentados em
`firmware/lyke_stm32/BATERIA-R11.md` e `scripts/manual_lyke/README.md`.
A instalação inicial do carregador V2 exige USB uma vez; com a base instalada,
as próximas atualizações do STM usam `lyke-stm32-ota.bin` no portal do ESP.
O repositório contém fontes e documentação; binários reais ficam no ambiente local.

## Credenciais e cópia de publicação

Este repositório é uma cópia dos fontes preparada por `scripts/prepare_private_repo.py`.
Chaves OpenAI, tokens Supabase/GitHub, senhas Wi-Fi, fotos privadas, backups de flash/NVS
e binários reais ficam somente no workspace local e nos serviços apropriados.
Senhas de AP em fontes e documentos históricos foram substituídas por marcadores.
Os fontes antigos da montagem S3 podem exigir restaurar suas configurações locais próprias.
Nunca grave uma imagem da montagem S3 na ESP32-CAM clássica.

A chave OpenAI fica nos segredos da Edge Function. A calculadora guarda em NVS apenas
sua credencial de acesso ao serviço. O bucket de fotos é privado.

## Arquitetura

```mermaid
flowchart LR
  K[Teclado e LCD] <--> S[STM32 Lyke R11]
  S <-- UART 115200 --> E[ESP32-CAM Lyke R8]
  E -- HTTPS e credencial do aparelho --> F[Supabase Edge Function]
  F --> B[Storage privado: JPEGs originais]
  F --> D[Postgres: consultas e respostas]
  F --> O[OpenAI Responses: GPT-6.1 Terra]
```

O arquivo também preserva fontes e notas da calculadora anterior S3 + DevKit.
Ela é uma montagem diferente e está identificada separadamente na base de conhecimento.
