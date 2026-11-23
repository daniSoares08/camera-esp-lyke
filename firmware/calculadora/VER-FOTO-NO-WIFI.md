# Ver as fotos da calculadora

1. Capture com **SPC**, aguardando cada quadradinho terminar de piscar.
   Pode capturar **uma, duas ou três fotos**. Não precisa preencher os três.
2. Pressione **E → Ver foto no Wi-Fi**.
3. Conecte o computador/celular à rede **CalcFoto-xxxx**, senha **CHANGE_ME_ESP_AP**.
4. Abra **http://192.168.8.1/foto**. Após atualizar o firmware, recarregue a página
   para receber a nova interface.
5. Use **Foto 1**, **Foto 2** e **Foto 3** para ver e baixar cada JPEG original.
   **Última captura** mostra a imagem capturada mais recentemente.

Escolher Foto 1/2/3 muda apenas a visualização. **E → Enviar fotos para IA**,
ou **Consultar API (fotos selecionadas)** na página, envia todas as fotos dos
quadradinhos juntas, em ordem, na mesma consulta. Se houver apenas uma ou duas,
envia somente essas. Sem fotos, solicita uma captura antes de enviar.

**Tirar foto sem API** acrescenta uma foto ao conjunto, até três, sem internet
ou consumo de créditos. No menu E, **Remover última foto** permite refazer a
última; **Limpar fotos** esvazia o conjunto. Uma falha de rede/API preserva as
fotos para tentar novamente; uma resposta válida esvazia o conjunto.

As fotos ficam em PSRAM e desaparecem ao desligar, reiniciar ou atualizar o
firmware. A última captura pode permanecer disponível depois de limpar/enviar
o conjunto, até ser substituída ou até reiniciar. As imagens da página são os
JPEGs usados na consulta, sem miniaturas nem novas capturas. Uma transferência
em andamento mantém referência própria ao buffer.

A página mostra o estado da câmera. Depois de copiar cada JPEG, o firmware
solicita standby do sensor, confere o registrador quando possível, encerra o
driver e para XCLK. Isso ocorre antes da consulta à API. Abrir a página,
consultar o estado ou baixar imagens não inicializa a câmera. Não existe corte
físico da alimentação do módulo; nenhum consumo zero foi medido ou alegado.
O Wi-Fi permanece ativo enquanto o visualizador está aberto.

Para consultar a IA, conecte também a calculadora a uma rede salva pelo seletor
da página ou pelo menu Wi-Fi. Ela opera como ponto de acesso e estação ao mesmo
tempo; a troca de canal durante busca/conexão pode exigir reconectar o navegador
à rede CalcFoto. O computador pode usar Ethernet para internet e Wi-Fi apenas
para falar com a calculadora. O diagnóstico informa o código da última falha
na tentativa de conexão, sem expor senhas ou chave de API.

O portal de fotos/configuração fecha após dez minutos **sem acesso**; as consultas
da página renovam esse prazo. O modo **Atualizar firmware** é separado e permanece
aberto até sair ou concluir o envio, com a mesma senha **CHANGE_ME_ESP_AP** nas duas placas.
Nenhum GPIO foi alterado.
