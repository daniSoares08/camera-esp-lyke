# Gerar o manual do comprador

Em 24/11 os fontes atuais passaram à revisão preparada R12/R9. O PDF/MD
edição 2.0 continua sendo o artefato da R11/R8 instalada. Para reproduzir
essa edição exatamente, usar o snapshot do commit `a9bb93f` e as instruções
abaixo. A revisão preparada está descrita em `docs/REVISAO-R12-DESENHOS-E-WIFI.md`;
não rotular os fontes atuais como R11 ao gerar uma edição histórica.

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

Edição final 2.0: STM R11 + ESP R8. Antes de gerar, compile a R11 e rode
`firmware/lyke_stm32/tests/run_native.py` para criar os frames reais de
notação matemática e bateria. A primeira edição continua no histórico.
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
