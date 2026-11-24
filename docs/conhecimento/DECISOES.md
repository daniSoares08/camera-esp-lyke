# Preferências e decisões do usuário

## Produto

- Ainda em 24/11, usuário ligou e autorizou a instalação. STM R12 e ESP
  R9a instalados por Wi-Fi/UART, revisões/MAP49 conferidos; configurações
  e resposta preservadas. R9a oculta rodapé legado de tokens no leitor.
  Manual sincronizado para edição 3.0. Sem nova chamada paga à API.

- Em 24/11/2026, reaberto o projeto: verificar rede Visitantes salva,
  Wi-Fi sem símbolo persistente (três pulsos no sucesso; X/sinal alternados
  na falha), figuras roláveis entre passos da resposta. Usuário confirmou
  que a rede aberta da universidade tem internet direta. Ele ligará a
  calculadora depois; preparar sem gravar. API v3 compatível publicada,
  sem chamadas pagas. Novo conjunto preparado: STM R12 + ESP R9.

- Em 23/11/2026, usuário corrigiu divisor para dois 470 kilohms e pediu
  bateria desenhada à esquerda dos quadrados das fotos. Confirmou a R11
  instalada, ícone aparecendo e tensão normal. Tornou o GitHub público e
  autorizou atualizar os fontes/documentação, mantendo credenciais/binários
  privados. Depois pediu manual PDF e Markdown finais e encerrou o projeto.

- Ainda em 23/11/2026, usuário conectou o STM, autorizou gravar e entrou em
  DFU. Base R10 instalada por USB; ESP R8 instalada por OTA; STM R10a
  instalada pelo Wi-Fi/UART para comprovar a atualização efetiva. Usuário
  confirmou menu/teclas e depois raiz/frações completas dentro da moldura.
  A janela continua a mesma: 126×48 pixels úteis, duas linhas superiores
  do LCD fora de uso, seis linhas visíveis; cabeçalho só fora do leitor.

- Em 23/11/2026, solicitado preparar atualização do STM pela UART/Wi-Fi do
  ESP sem soldar fios adicionais e melhorar a notação da calculadora local:
  frações empilhadas, raízes com barra, expoentes e símbolos gregos legíveis.
  O usuário conectará o STM depois para testar. Não gravar nesta preparação.
  Base preparada R10 exige instalar carregador V2 + app juntos pelo USB uma
  vez; depois confirmar OTA real usando as ligações existentes.
  Aceito descartar fotos temporárias do ESP se precisar da memória para
  atualizar; implementação só as descarta em caso de falta de PSRAM.
  Manual e diagnóstico físico da bateria continuam adiados.

- Ajustes solicitados durante o teste físico de 23/11/2026: ao ler resposta,
  usar todas as seis linhas só para conteúdo, sem ícones, número de página
  ou título. No restante do uso, manter cabeçalho normal. A API deve
  entregar fórmula, substituição e desenvolvimento necessário para uma
  resposta aceita em avaliação, sem diálogo nem gabarito isolado. Evitar
  linhas vazias e limitação artificial de palavras. Revisar o manual só
  após os ajustes. Diagnóstico físico dos 6,66 V da bateria fica por último:
  o usuário não quer abrir a carcaça para medir agora.

- Em 23/11/2026, solicitado manual ilustrado em PDF voltado ao comprador
  iniciante, acompanhado de Markdown completo para aprender com IA.
  Cobrir todas as funções e guias OpenAI/Claude, deixando explícito que
  só OpenAI foi testada. Manual gerado em `output/pdf/`, fontes do manual
  em `scripts/manual_lyke/`. Não apresentar desenho como fotografia real.

- Em 23/11/2026, usuário autorizou configurar OpenAI/API, guardar as fotos
  e criar um repositório privado no GitHub com fontes e esquema. Escolheu
  uma organização Supabase separada chamada CAMERA_ESP. Arquivo de imagens
  privado; chave OpenAI somente no servidor; credencial própria do aparelho.
- Preferência informada: “6.2 Terra”. Catálogo consultado oferece Terra como
  GPT-6.1 Terra, que foi configurado com raciocínio alto. O formato deve ser
  preciso e legível no LCD de 21 colunas, com conferência e sem inventar dados.
- A solicitação atual autorizou os testes mínimos pagos da integração. Não
  transformar isso em autorização para consultas pagas periódicas/automáticas.
- Em 22/11/2026, o usuário pediu Wi-Fi de atualização da Lyke sempre ativo,
  sem expiração. Desligamento pelo STM só vale na sessão atual; reiniciar o
  ESP deve reabrir `Calc-Lyke` automaticamente, inclusive após OFF.
- Na Lyke 101078, o usuário dispensou a tecla ON e desliga pela chave da
  bateria. O mapa tem 49 contatos, menu em MODE e captura em SHIFT + direita.
  O LCD usa a largura inteira, 21 colunas; somente as duas linhas do topo
  ficam fora da janela por causa da moldura. Contraste 5 e rotação a 180°.
- As preferências E/SPC e as perdas M/S/ALPHA listadas abaixo pertencem
  à montagem antiga HP48. Preservar a separação entre as duas calculadoras.
- Para a nova calculadora, reaproveitar o teclado original de 19 trilhas e
  descobrir o mapa. O usuário aceita PCF8575 e prefere dispensar o FT4232H
  da montagem autônoma. A manutenção do firmware deve ser pelo Wi-Fi.
- Calculadora comum/algébrica como padrão, RPN como opção.
- Todas as teclas mapeadas devem funcionar como calculadora. Perdas M, S e
  ALPHA foram aceitas; oferecer funções ausentes pelo menu/seletor.
- A ideia inicial ENTER por 3 s para menu foi **substituída por tecla E**
  exclusiva de menu. Não restaurar o comportamento antigo como padrão.
- **SPC captura** uma foto por toque; até três no mesmo prompt, mas uma ou
  duas continuam válidas. Aguardar captura anterior. Envio à IA por ação
  explícita de menu, distinto de captura e visualização.
- Cabeçalho: três quadradinhos sempre presentes, piscando na captura e cheios
  ao concluir. Remover texto ALG/DEG/OFF/WIFI do cabeçalho; ícone mínimo Wi-Fi
  à esquerda só quando conectado.
- Apagar funções como `tan(` por inteiro, não letra a letra. π deve ter símbolo.
- Resposta de matemática em texto compatível LCD e rolável por setas.
- Capturar foto da questão não deve exigir AF no módulo de foco fixo.
- Câmera da calculadora deve inicializar só para captura e voltar a standby;
  painel de bancada é exceção funcional, pois mantém prévia contínua.
- Mostrar/baixar os JPEGs originais pela rede local, independentemente da API.
- Fotos antigas perdidas não precisam ser recuperadas; capturas futuras bastam.
- Procurar redes deve listar redes reais; perfis salvos e cadastro extra.
- Casa: DANIEL-HUAWEI. Celular atual: IPHONEPROMAX; substitui
  “iPhone 17 PRO MAX de Daniel”. Credenciais já fornecidas ficam na configuração
  privada/NVS; não duplicar em documentação pública.
- Senha fixa dos APs locais solicitada: **CHANGE_ME_ESP_AP**. OTA dos dois processadores
  sem limite de dez minutos enquanto a opção estiver aberta.
- Em 16/11/2026, o usuário determinou que o acesso à atualização Wi-Fi
  deve funcionar pelo próprio aparelho após reinício. Testar a abertura
  do portal com os conectores de manutenção removidos antes de considerar
  a montagem pronta para fechar. A versão Lyke R3 instalada abre o AP no boot,
  confirmado após reinício da OTA em 22/11. Teste com alimentação fria pendente.
- Tela antiga em 180°, devido ao encaixe físico. Diminuir consumo dos LEDs
  quando viável; usuário aceita retirada de LED para liberar ADC.
- Bateria máxima 4,2 V; divisor desejado exatamente 2 × 470 kΩ.

## Trabalho e comunicação

- Português brasileiro, respostas claras e prática de executar o solicitado.
- Usuário autorizou uso do computador, instalação dos firmwares solicitados e
  substituição do programa existente nos respectivos momentos. Não inferir
  disso autorização para gravar alvo não identificado ou fazer envios pagos
  por rotina. Pedidos futuros concretos determinam a ação.
- Internet do computador pelo **cabo de rede**, usando Wi-Fi só para a
  calculadora. Houve erros `{"detail":"Bad Request"}` no contexto da conexão;
  não atribuir automaticamente esse texto ao firmware/API sem diagnóstico.
- Prefere esquema bonito em **HTML**, com desenhos da placa, LCD e placa
  azul, cores de fios, origem/destino e resistores. Entregar arquivo offline.
- Atalho de câmera na área de trabalho: **ESP-CAM**, ícone de câmera.
- Preservar o que já foi feito e sinalizar pendências reais; não solicitar
  repetidamente as mesmas provas físicas quando já houver dados suficientes.

## Escopo da preservação

Esta base é uma consolidação técnica da conversa e dos arquivos locais, não
uma transcrição literal. Não guarda login/cookies/chave de API. Não promete
acesso à memória desta conversa em outro chat: o próximo chat deve abrir
estes arquivos. Guias antigos podem conter opções substituídas; verificar
decisões atuais e firmware efetivo antes de executar comandos.
