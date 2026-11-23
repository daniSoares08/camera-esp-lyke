# Mapeador do teclado HP 48

Conecte a placa pelo conector **USB/UART (CH343)**. Deixe o cabo **OTG desconectado**: GPIO19 e GPIO20 estão no teclado. O programa anterior é substituído; a câmera não é inicializada.

Abra `Abrir mapeador do teclado.bat` na raiz do projeto. A tela está em http://127.0.0.1:8766 e a conexão serial é automática (115200 baud). Não abra outro monitor serial ao mesmo tempo.

1. Aguarde “ESP conectado” e clique em **Iniciar mapeamento**.
2. Aperte **A**, a primeira tecla sob o visor. Solte-a completamente.
3. Siga da esquerda para a direita: A–F, G–L, M–R, S–X, ENTER/Y até apagar, α até ÷, shift esquerdo até ×, shift direito até −, ON até +.
4. A tela destaca a próxima posição. Não é preciso usar shift ou α para registrar letras: aperte apenas a tecla física indicada.
5. Ao terminar, use **Testar teclas** e exporte JSON, CSV ou C++.

São 49 posições. Clique em uma tecla do desenho e depois em **Iniciar** para corrigi-la. **Desfazer** remove a posição anterior e pausa; **Pular tecla** deixa uma pendência. Contatos já atribuídos a outra posição e vários contatos simultâneos não avançam a sequência. A soltura completa é obrigatória entre teclas.

O mapa é salvo a cada registro em `outputs/mapa_teclado/mapa.json`; reiniciar a interface retoma a primeira posição ainda não visitada. Posições puladas continuam pendentes e precisam ser selecionadas para completar. O arquivo JSON diferencia tecla ausente, pendente (`null`) e mapeada. No C++ exportado, `b=-1` representa GND, e `a=b=-2` indica posição ainda sem mapa. Pares não possuem orientação fixa de linha/coluna: o mapa registra os dois contatos reais de cada tecla.

GPIOs varridos: **0, 1, 2, 3, 14, 19, 20, 21, 38, 39, 40, 41, 42, 45, 46, 47, 48**. Todos são varridos nos dois sentidos quando o SDK informa capacidade de saída. O scanner usa uma única saída open-drain em LOW por vez, com todas as outras linhas como entrada com pull-up, e aguarda 500 µs após acionar e soltar cada linha. Os pinos da câmera, PSRAM/flash e UART43/44 não são varridos. Os GPIO0/3/45/46 afetam o boot: não mantenha teclas apertadas ao ligar ou resetar a placa. Se uma tecla não gerar evento, pule e confira sua ligação; não é possível inventar o contato ausente por software.

## Compilar e gravar novamente

No PowerShell, a partir da raiz:

```powershell
& .\firmware\mapear_teclado\Compilar.ps1
& .\firmware\mapear_teclado\Gravar.ps1 -Porta COM12
```

Troque COM12 pela porta USB/UART atual, se necessário. Compilação: core ESP32 instalado no projeto, ESP32-S3, flash 16 MB, PSRAM OPI, CDC no boot desabilitado, Serial via UART0. A gravação inclui bootloader, partições e aplicativo.

Protocolo serial: linhas JSON do ESP; comandos `HELLO`, `START`, `STOP`. O scanner só começa após `START`. Eventos `down`, `up`, `ambiguous` e heartbeat `hello` a cada segundo. O filtro vota nas últimas 24 varreduras: exige 6 leituras do mesmo contato, limita sinais concorrentes e exige soltura antes da próxima tecla. Tolera contatos intermitentes como o medido em C (cerca de 50% das amostras). Um autoteste executado no ESP verifica oscilação, ruído, contato mantido, soltura e ambiguidade; `filter_selftest` informa o resultado. A interface pausa após reconexão/reset e mantém o arquivo salvo. O servidor escuta somente em 127.0.0.1; não usa Wi-Fi nem serviço externo.

Ao ligar, o firmware calibra os níveis baixos permanentes com todas as teclas soltas (64 amostras). Na montagem real foi medido GPIO48 baixo em repouso, e A fecha GPIO40–GPIO42. O nível fixo do 48 é descontado para não gerar falso contato simultâneo. Se uma tecla ligar outra entrada a uma linha já baixa, ela aparece como contato com GND. O heartbeat e a interface mostram os pinos descontados; recalibre com RESET, sempre sem apertar teclas.

Verificações de lógica com contatos sintéticos: `.venv\Scripts\python.exe -m unittest discover -s teclado -v`. Elas não substituem o teste físico de cada tecla.

Referência de GPIOs/USB/memória: https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/peripherals/gpio.html
