# Gerar o manual do comprador

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
