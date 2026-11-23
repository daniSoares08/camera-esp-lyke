# Revisao de 03/11/2026

E abre o menu em qualquer tela. ENTER confirma/calcula sem espera. SPC captura uma foto para a consulta, sem exigir Wi-Fi ou gastar creditos. Ate tres fotos ficam em PSRAM; cada captura pisca o proximo quadradinho e o preenche somente depois de copiar o JPEG e desligar a camera.

O cabeçalho mostra somente o pequeno simbolo de Wi-Fi quando conectado, os tres indicadores e, em menus, o titulo resumido. Na calculadora nao mostra ALG/DEG/OFF. O desenho pi usa glifo proprio, e a expressao armazena a constante simbolica pi para calcular com a precisao interna. BACKSPACE remove nomes de funcoes e parenteses de abertura juntos; numeros continuam apagando digito a digito.

Depois de capturar de uma a tres fotos, E → Enviar fotos para IA envia todas juntas na ordem. Uma ou duas fotos sao suficientes: nao e obrigatorio preencher os tres quadradinhos. Remover ultima foto permite refazer a ultima captura; Limpar fotos inicia outro conjunto. A quarta captura e recusada. Em caso de erro de rede/API, as fotos permanecem; apos uma resposta valida, o conjunto e esvaziado. A ultima imagem do visualizador pode continuar disponivel. Reiniciar perde todas as fotos em RAM.

A pagina /foto possui botoes Foto 1/2/3 para ver e baixar cada JPEG. A selecao na galeria nao muda o conjunto enviado a IA. Abrir a galeria nao liga a camera; o estado de standby fica visivel na pagina.

O JSON usa varios input_image no mesmo input de usuario. O corpo e transmitido em fluxo com base64 incremental; nao e criada uma copia expandida de todas as imagens. Nenhuma consulta paga foi feita nos testes locais.

IPHONEPROMAX passa a ser provisionado em novas instalacoes e migrado para NVS nas existentes, sem apagar a chave da API nem a rede de casa. No iPhone, ative Maximizar Compatibilidade (rede 2.4 GHz/WPA2). Nomes com espacos sao aceitos; a faixa e o modo de seguranca tambem influenciam a conexao.

Busca de redes: inicializacao estabilizada, varredura assincrona, espera ate concluir e erro explicito para falha/timeout, em vez de interpretar erro como lista vazia. O teste real encontrou nove redes, incluindo IPHONEPROMAX. A conexao ao hotspot ainda nao foi confirmada. A revisao final limpa a tentativa anterior antes de conectar, interrompe repeticoes depois de falhar e registra estado/motivo do driver para o proximo teste.

OTA: sem prazo de dez minutos; encerra por saida explicita/reinicio apos upload. A versao do DevKit tambem precisa ser instalada para remover o prazo no atualizador da tela. O portal de fotos/configuracao conserva seu prazo por inatividade, pois e uma funcao diferente.

A versao anterior desta revisao ja foi instalada por Wi-Fi no S3 e teve tres JPEGs distintos conferidos, com camera inativa e standby confirmado. O DevKit aceitou sua atualizacao por Wi-Fi; a conferencia do slot apos o reinicio ainda esta pendente. A revisao final de diagnostico aguarda envio ao S3: a bateria acabou e o usuario pediu para preparar os arquivos durante a carga. Nao foram feitas consultas pagas nestas verificacoes.
