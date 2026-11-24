# API e arquivo privado de fotos — 23/11/2026

**Atualização de 24/11:** API v3 publicada com documento gráfico opcional
(`display=lyke-graphics-v1`), mantendo clientes antigos em texto. Schema,
comandos e limites em `supabase/functions/lyke-solve/graphics.ts`.
Sem alteração de tabelas/RLS/bucket/segredos. Dezesseis testes locais e
smoke GET/401 aprovados; nenhuma nova chamada paga. Geração real de figuras
no aparelho ainda aguarda teste. STM R12/ESP R9a instalados por Wi-Fi em
24/11, revisões e mapa de 49 teclas conferidos; resposta salva preservada.

**Edição atual:** STM R12/ESP R9a, manual 3.0 em `docs/manual/`. O usuário
tornou o repositório público; o arquivo de fotos permanece privado e as
credenciais continuam fora do Git. Os detalhes abaixo registram a integração
e os testes, incluindo a revisão v2 do serviço. Somente OpenAI integrada/testada.

## Serviços configurados

- Conta OpenAI no navegador: saldo de US$ 10 confirmado antes dos testes; duas chamadas reais aprovadas.
- Modelo principal: `gpt-6.1-terra`, esforço `high`. Econômico: `gpt-6-luna`, esforço `medium`.
- Organização Supabase: **CAMERA_ESP**, plano Free.
- Projeto: **camera-esp-lyke**, região **São Paulo / sa-east-1**.
- Ref: `vxjmoctaomihrmxmuxip`.
- Função HTTPS: `https://vxjmoctaomihrmxmuxip.supabase.co/functions/v1/lyke-solve`.
- Bucket privado: `lyke-images`.
- Tabela de consultas: `public.lyke_requests`.

O Supabase aberto no Brave é uma conta diferente da conexão MCP preexistente PUC_FILES.
Os recursos deste projeto foram criados na conta aberta pelo usuário no Brave,
com um token de administração limitado a este projeto e com expiração de sete dias.
A expiração desse token de implantação não interrompe a função em execução.

## Fotos e respostas

A função aceita de uma a três fotos JPEG, até 512 KiB por imagem. Primeiro guarda os
originais no bucket privado, depois chama o modelo. Se o arquivo falhar, não chama
a OpenAI. As fotos continuam arquivadas quando a OpenAI recusa ou falha.

Cada consulta guarda modelo, estado, data, caminhos das fotos, resposta, texto do LCD,
ID da resposta OpenAI e contagem de tokens. A imagem pode ser baixada sem recompressão.
Um ID de consulta evita nova cobrança quando o aparelho recupera uma resposta já concluída.
Há um limite inicial de 50 novas consultas por aparelho nas últimas 24 horas, ajustável
no segredo `LYKE_DAILY_LIMIT`. Esse limite não é uma previsão de consumo dos US$ 10.

As capturas locais ficam em RAM até enviar à IA. Desligar antes do envio perde essas
capturas. O arquivo de fotos já enviadas e as respostas salvas no Supabase persistem.
Não há eliminação automática de fotos configurada.

Abra o [Storage do projeto](https://supabase.com/dashboard/project/vxjmoctaomihrmxmuxip/storage/buckets)
com a conta Supabase correta e entre em `lyke-images`. Os caminhos seguem
`lyke-01/<id-da-consulta>/1.jpg`. A tabela correspondente fica no Table Editor.
O bucket não disponibiliza URLs públicas de leitura.

## Precisão e tela

O servidor controla as instruções: transcrição dos dados, resolução, conferência por
substituição/unidades/domínio e resultado. Fotos ilegíveis ou sem questão devem gerar
pedido de outra foto, sem inventar dados. O texto é convertido para ASCII e dividido
em linhas de até 21 caracteres, compatíveis com o leitor de quatro linhas do STM R8.
A IA pode errar; as instruções e os testes melhoram a verificabilidade, sem garantia
de precisão absoluta.

O nome encontrado no catálogo foi GPT-6.1 Terra; GPT-6.2 aparece nas variantes Sol e Astra.
Os modelos ficam centralizados em `supabase/functions/lyke-solve/core.ts`.

## Firmware e configuração do aparelho

ESP `lyke-r5` instalado por OTA em app1 (`0x190000`), com versão e digest conferidos
após reinício. STM32 continua `lyke-r8`, com 49 contatos reconhecidos.
O NVS da Lyke recebeu a URL do serviço, a credencial exclusiva do aparelho e o perfil
local DANIEL-HUAWEI. Não contém a chave OpenAI do servidor.

O portal inicia ligado a cada boot, sem expiração, e o ESP tenta a última rede externa
que conectou com sucesso. MODE → Desligar Wi-Fi mantém o rádio desligado só até o
próximo reinício. A fila de imagens é preservada quando ocorre erro na consulta.

No portal local, a seção **IA e arquivo de fotos** permite salvar URL/credencial,
ativar o modo econômico, enviar as fotos e abrir a última resposta.

## Segredos e publicação

No workspace original, os arquivos de configuração estão em `.private/lyke-cloud/`:
chave OpenAI, token de implantação Supabase, senha do banco, credencial do aparelho,
relatórios e JPEGs de teste. `.private/ap-password.txt` contém a senha do AP.
Esses arquivos não entram no Git, nem os binários de firmware e dumps de memória.
Nunca publique uma chave `service_role`, `sb_secret`, OpenAI ou token de dispositivo.

A função usa `OPENAI_API_KEY`, `LYKE_DEVICE_TOKEN_SHA256`, `LYKE_DEVICE_ID` e
`LYKE_DAILY_LIMIT`, além das credenciais administrativas disponibilizadas pelo runtime
Supabase. Só o hash da credencial do aparelho fica no servidor para conferência.
`verify_jwt=false` é intencional: o handler exige e valida a credencial própria do
aparelho antes de acessar banco, Storage ou OpenAI. Isso é coberto pelos testes.

## Implantar novamente

CLI validado: **Supabase 3.2.0**. Use um PAT limitado ao projeto com leitura de Project
Settings, Advisors, API Keys e API Key Secrets, e escrita de Database, Edge Functions
e Edge Function Secrets. Não use o token de outro projeto/conta.

```powershell
npx --yes supabase@3.2.0 secrets set --remote=vxjmoctaomihrmxmuxip --env-file .private/lyke-cloud/edge-secrets.env
npx --yes supabase@3.2.0 functions deploy lyke-solve --remote=vxjmoctaomihrmxmuxip --use-api --no-verify-jwt
```

Forneça `SUPABASE_ACCESS_TOKEN` pelo ambiente, sem colocar o valor no terminal,
em argumentos de comando, commits ou capturas de tela. A CLI v3 pede confirmação
para mudanças remotas. Os dois SQLs em `supabase/migrations/` foram aplicados ao novo
banco via Management API; a tabela de histórico da CLI não foi usada nesta implantação.
Em um projeto vazio, aplique-os na ordem. Não reaplique o primeiro sobre este banco.

## Validação concluída

- Oito testes locais: autenticação, JPEGs e limites, bloqueio de URLs externas,
  instruções do servidor, formato LCD, ordem de arquivo/chamada, cache e limite de uso.
- Consulta real de visão com imagem sintética `2*x+3=11`: resultado `x=4`, com conferência.
- Reenvio do mesmo ID retornou cache idêntico.
- JPEG do arquivo baixado com o mesmo SHA-256 do original.
- Tabela com RLS; `anon` e `authenticated` sem leitura; bucket privado.
- Auditoria Supabase sem avisos após restringir o helper privilegiado de RLS da plataforma.
- Teste completo do aparelho: câmera capturou um JPEG de 409.062 bytes, enviou ao
  serviço, arquivou e recebeu resposta pelo ESP. A foto mostrava a bancada, sem
  exercício: o modelo pediu um enunciado em vez de inventar uma resolução.
- Consulta pelo aparelho: 3.295 tokens de entrada e 340 de saída.

Relatórios detalhados ficam na pasta privada. Não houve mudança no STM32 nesta etapa.
Ainda falta aferir resolução de questões reais no LCD pelo usuário, fazer uma sessão
de três fotos e validar a OTA física do STM32 pelo ESP.

Referências: [modelo Terra](https://developers.openai.com/api/docs/models/gpt-6.1-terra),
[visão](https://developers.openai.com/api/docs/guides/images-vision),
[segredos Supabase](https://supabase.com/docs/guides/functions/secrets),
[controle de acesso do Storage](https://supabase.com/docs/guides/storage/security/access-control).

## Repositório privado

Fontes e esquema: [daniSoares08/camera-esp-lyke](https://github.com/daniSoares08/camera-esp-lyke).
A cópia local de publicação fica em `.private/github_repo/`; o workspace original
continua preservado. Atualizações usam `scripts/prepare_private_repo.py`, seguidas
de revisão e commit nessa cópia. Credenciais e fotos do arquivo não são publicadas.
