# Atualização por Wi-Fi dos dois ESPs

Instalação inicial: gravar **os dois** por USB usando `Gravar.ps1`. Isso instala
as novas tabelas de partições. Depois use somente o arquivo de aplicativo por
Wi-Fi: não envie bootloader, partitions, boot_app0 ou merged.

1. Pressione E e escolha **Atualizar firmware**.
2. Escolha **Atualizar ESP32-S3** ou **Atualizar tela/DevKit**.
3. No computador ou celular, conecte à rede mostrada no LCD:
   `CalcS3-xxxx` ou `CalcTela-xxxx`, com a senha exibida. Aceite permanecer
   conectado mesmo sem internet. O modo permanece aberto até sair ou concluir a atualização.
4. Abra **http://192.168.8.1** no navegador, usando HTTP.
5. Selecione o arquivo correspondente e pressione **Enviar firmware**:

| Placa | Arquivo |
|---|---|
| ESP32-S3 | `bin/s3_calculadora/s3_calculadora.ino.bin` |
| DevKit da tela | `bin/devkit_display/devkit_display.ino.bin` |

Espere a confirmação e o reinício. Não desligue a alimentação durante o envio.
Reconecte o computador à sua rede normal depois. No S3, ON cancela o modo de
atualização antes do envio. O DevKit retorna ao normal ao reiniciar, pelo encerramento explícito ou pelo comando USB `n`.

Não há alteração de GPIOs. O comando para iniciar o modo da tela usa o mesmo
fio S3 TX43 → DevKit RX16, com tipo de pacote separado e CRC. No modo normal,
o Wi-Fi do DevKit permanece desligado. A câmera e a API não são acionadas pela
atualização.

## Memória e validação

S3: dois espaços de 3 MiB. DevKit: dois espaços de 1,5 MiB. NVS, otadata,
LittleFS e coredump mantêm os endereços anteriores. O firmware atual cabe nos
espaços; futuras compilações devem respeitar esses limites.

O servidor exige a senha WPA da rede temporária e um token na requisição de
envio. Recusa nome de arquivo incorreto, chip incompatível, cabeçalho inválido
e imagem que falhe na validação do ESP-IDF. O verificador de bancada também
envia MD5 para testar corrupção durante a transferência. O arquivo não é
assinado digitalmente; o nome e chip não garantem que qualquer firmware
renomeado seja desta calculadora. Envie somente os binários preparados para
este projeto. O modo não é publicado na internet ou na rede doméstica.

Uma transferência interrompida antes da conclusão não substitui a aplicação
em execução. **Não há rollback automático de uma aplicação que grave
corretamente, mas falhe ao iniciar.** Mantenha uma forma de recuperação por
USB/BOOT acessível na carcaça. Este recurso permite os testes normais com a
carcaça fechada, mas não elimina a necessidade eventual de recuperação física.

A estrutura segue as [partições OTA do ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html).
Reparticionamento e gravação de sistema de arquivos continuam sendo feitos
por USB; o atualizador Wi-Fi atualiza apenas a aplicação.

## Verificação de bancada

`verificar_ota.py --target s3|display --restore-profile NOME` usa USB para
ativar a rede e ler o estado após o reinício; o binário é transmitido por
HTTP/Wi-Fi. Testa autenticação, arquivo errado, chip errado, corrupção e
troca efetiva de slot. Restaura o perfil de Wi-Fi do computador ao terminar
e remove o perfil temporário. Relatórios em `outputs/calculadora/ota_*`.
