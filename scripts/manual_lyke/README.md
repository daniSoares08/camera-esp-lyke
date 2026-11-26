# Gerar o manual do comprador

## Edição 4.0 finalizada em 26/11/2026

STM R14/ESP R12 finais instalados; revisões, MAP49, resposta, padrão 60 min e
ciclo SHIFT+AC/MODE conferidos. Sono físico confirmado pelo usuário e retorno
ESP deep-sleep/ext0. Ensaio Wi-Fi 20 min dispensado pelo usuário; cobertura nativa.
Manual final em output/pdf e docs/manual, 36 páginas renderizadas e conferidas,
PDF/MD/imagens sincronizados. ZIP portátil em output/pdf. Somente OpenAI testada.

Para regenerar a edição entregue, use `--verified-installation`. Sem essa opção,
o gerador produz prévia separada e identificada em output/pdf-previa-r14.
Para nova revisão, confira instalação/funcionalidade antes de usar a opção.
O histórico abaixo preserva a preparação anterior à instalação final.

## Histórico de preparação

## Edição 4.0 preparada em 25/11/2026

Fontes STM R14 + ESP R12: sono por SHIFT+AC, automático após 60 minutos sem
botão, ajuste de 1 a 1440 minutos, tentativas Wi-Fi limitadas a 20 minutos
desconectado, pausa de rádio na câmera e símbolos no leitor IA.
**Instalação final pendente de recuperação física após ensaio de diagnóstico.**
Não substituir a entrega pública por uma alegação de instalação concluída.

O comando normal agora gera uma **prévia identificada na capa e nos rodapés**
em `output/pdf-previa-r14/`, preservando o manual entregue 3.0 em `docs/manual/`.
São 36 páginas, imagens, MD integral e ZIP portátil. Os frames vêm dos testes
nativos R14. Guias oficiais de conta/chave/cobrança OpenAI e Claude e preços
OpenAI foram reconferidos em 25/11; demais referências mantêm a consulta anterior.

Depois de instalar/conferir R14/R12 finais, verificar ausência de instrumentação
e concluir os ensaios possíveis:

```powershell
.venv\Scripts\python.exe scripts/manual_lyke/gerar_manual.py --verified-installation
```

Somente esse comando escreve em `output/pdf/`; depois renderize e confira de
novo, sincronize PDF/MD/imagens com `docs/manual/` e publique o snapshot sem
credenciais. A opção é uma declaração de evidência conferida, não faz flash
nem comprova por si só a instalação. Estado em `docs/REVISAO-R14-TEMPORIZADORES.md`.

## Histórico da entrega anterior

Edição 3.0, 24/11/2026: STM R12 + ESP R9a instalados por Wi-Fi.
Inclui aviso Wi-Fi, texto e desenhos roláveis. As figuras são exemplos
determinísticos do renderizador; geração real pelo modelo ainda não foi
validada no aparelho. Edição 2.0 R11/R8 preservada no commit `a9bb93f`.

O conteúdo, as figuras determinísticas e a diagramação estão em
`gerar_manual.py`. Não lê credenciais, não acessa a calculadora e não chama
APIs pagas. A foto de referência comercial é baixada do fabricante apenas
se ainda não existir no diretório de imagens. Não é foto da unidade montada.

No Windows, com Arial disponível e a partir da raiz do projeto:

```powershell
.venv\Scripts\python.exe -m pip install reportlab pillow
.venv\Scripts\python.exe scripts/manual_lyke/gerar_manual.py
pdftoppm -r 120 -png output/pdf/Manual-Lyke.pdf tmp/pdfs/manual-lyke/pagina
```

Saídas: `output/pdf/Manual-Lyke.pdf`, `Manual-Lyke.md`,
`Manual-Lyke-Completo.zip` e a pasta `manual-assets/`.
O PDF tem 36 páginas com marcadores de capítulos, índice e fontes clicáveis.
O Markdown contém o mesmo conteúdo e um contexto estruturado adicional para IA.

Edição atual 3.0: STM R12 + ESP R9a. Antes de gerar, compile a R12 e rode
`firmware/lyke_stm32/tests/run_native.py` para criar os frames reais de
notação matemática, bateria e figuras do leitor. As edições anteriores continuam no histórico.
`--unit-photo caminho.png` permite incluir a foto fornecida pelo proprietário;
ela é copiada sem edição. O gerador também reutiliza a cópia já presente em
`docs/manual/manual-assets/`. A foto da unidade foi tirada antes do ícone R11,
e isso aparece na legenda. Somente OpenAI integrada/testada; Claude permanece
como guia de preparação de conta para uma futura integração.

Antes de entregar uma nova edição:

- Conferir funcionalidades no firmware realmente instalado, sem misturar
  ESP32 clássico com a montagem GOOUUU S3.
- Preservar as distinções entre implementado, compilado e testado fisicamente.
- Consultar novamente os documentos oficiais de contas/API/preços.
- Não apresentar Claude como integrado: o servidor atual chama só OpenAI.
- Renderizar e inspecionar todas as páginas; o gerador rejeita margem inferior
  ultrapassada, mas isso não substitui a inspeção visual.
- Verificar os links relativos do Markdown e a integridade do ZIP.
- Conferir ausência de credenciais e de imagens privadas.

Revisão inicial de 23/11/2026: 36 páginas inspecionadas, 17 exemplos numéricos
executados no parser `scientific.h` em compilação nativa, sem gravação de firmware.
Simulações usam a fonte 5x7 do firmware, com a licença Adafruit incluída no pacote.

Revisão de 24/11/2026: 36 páginas renderizadas/inspecionadas, sem transbordamento;
PDF/Markdown/ZIP sincronizados. Guias de conta/API preservam consulta de fontes
de 23/11; a revisão alterou instruções locais e navegação do firmware.
