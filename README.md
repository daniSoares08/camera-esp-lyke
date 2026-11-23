# CAMERA_ESP — calculadora Lyke

Firmware, API e esquema da calculadora com **STM32G431CBU6 + ESP32-CAM clássica AI Thinker**.
[Repositório privado no GitHub](https://github.com/daniSoares08/camera-esp-lyke).

Estado verificado em **23/11/2026**: STM32 `lyke-r8`, ESP `lyke-r5`, 49 teclas mapeadas,
captura JPEG, resolução pela API e arquivo privado no Supabase funcionando.

## Começar

- [Estado e continuidade](docs/conhecimento/ESTADO-E-RETOMADA.md)
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
As fotos são arquivadas quando enviadas para resolução; uma captura offline fica em RAM
e precisa ser enviada antes de desligar para entrar no arquivo da nuvem.

## Compilação

ESP no Windows: Arduino CLI, core ESP32 **3.3.6**, ArduinoJson **7.4.2**, Python e esptool.
As ferramentas locais ficam em `.tools/` e `.venv/`; elas não são publicadas neste repositório.
Crie `.private/ap-password.txt` com a senha WPA2 do portal (8–63 caracteres ASCII) e execute:

```powershell
.\.venv\Scripts\python.exe firmware/esp32cam_integrada/build.py --profile lyke --revision lyke-r5
```

STM32 no WSL, com GCC ARM e Newlib Nano:

```sh
sh firmware/lyke_stm32/build.sh lyke-r8
```

Teste da API sem consumir créditos, com Node **24**:

```sh
node --test supabase/tests/cloud.test.ts
```

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
  K[Teclado e LCD] <--> S[STM32 Lyke R8]
  S <-- UART 115200 --> E[ESP32-CAM Lyke R5]
  E -- HTTPS e credencial do aparelho --> F[Supabase Edge Function]
  F --> B[Storage privado: JPEGs originais]
  F --> D[Postgres: consultas e respostas]
  F --> O[OpenAI Responses: GPT-6.1 Terra]
```

O arquivo também preserva fontes e notas da calculadora anterior S3 + DevKit.
Ela é uma montagem diferente e está identificada separadamente na base de conhecimento.
