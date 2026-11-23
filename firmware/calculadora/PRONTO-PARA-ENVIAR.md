# Firmware pronto para envio — 03/11/2026

Os testes locais passaram. A bateria acabou antes do próximo teste no aparelho.
Esta revisão final do S3 ainda NÃO foi enviada.

## Retomar quando carregar

1. Abra E → Atualizar firmware → Atualizar ESP32-S3 e avise no chat.
   Rede CalcS3-8428, senha CHANGE_ME_ESP_AP; página http://192.168.8.1.
   Arquivo: s3_calculadora.ino.bin. Enviar somente o aplicativo, sem merged,
   bootloader ou tabela de partições. Aguardar a confirmação e o reinício.
2. Capture uma ou duas fotos com SPC, aguardando cada uma, e abra
   E → Ver foto no Wi-Fi. Na rede CalcFoto-8428, senha CHANGE_ME_ESP_AP,
   abra http://192.168.8.1/foto e recarregue a página. Foto 1/2/3 permite
   ver/baixar cada original. A IA aceita qualquer conjunto de uma a três fotos.
3. Testar a conexão da calculadora a IPHONEPROMAX. A busca já encontrou a rede;
   a conexão ainda não foi confirmada. A nova revisão registra estado e motivo
   de falha para orientar o diagnóstico. Não é necessário consultar a API.
4. Conferir o slot do DevKit após o reinício. O binário da tela já teve envio
   OTA aceito nesta sessão; não é necessário reenviar apenas por rotina.
   Se for necessário: E → Atualizar firmware → Atualizar tela/DevKit,
   rede CalcTela-1d44, senha CHANGE_ME_ESP_AP, arquivo devkit_display.ino.bin.

Manter internet do computador pelo cabo e Wi-Fi apenas para a calculadora.
O modo de atualização permanece aberto até sair ou concluir o envio.
Fotos em RAM não sobrevivem a reinício/atualização.

## Conteúdo

- s3_calculadora.ino.bin: E/SPC, cabeçalho, π, exclusão por função,
  captura de 1–3 imagens, galeria, standby da câmera, Wi-Fi e diagnóstico.
- devkit_display.ino.bin: tela girada, senha fixa e OTA sem expiração.
- manifest.json: tamanho, chip e SHA-256 de cada aplicativo.

Pacote privado: os aplicativos incluem os perfis Wi-Fi solicitados.
Não há consulta paga automática ao iniciar, capturar ou visualizar.

Pacote: outputs/calculadora/firmware_wifi_2026-11-03.zip
