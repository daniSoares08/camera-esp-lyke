# Decoder do pacote STM32

`puff.c` e `puff.h`: Mark Adler, puff 2.3 (21/01/2013), copiados sem
alteração da distribuição zlib **v1.3.1**, diretório `contrib/puff/`.
Licença permissiva zlib preservada em `puff.h` e referenciada por `puff.c`.

Fonte: https://github.com/madler/zlib/tree/v1.3.1/contrib/puff

SHA-256 dos fontes originais:

- puff.c: `5b9d75aeb5baf3575415bc6ade3f2a02e50b6b971b3f8b4fda2b03543bc6e52f`
- puff.h: `969b7be2a930db0cdcb19b0e5b29ae6741f5a8f663b6dba6d647e12ec60cfa8e`

O formato do projeto limita cada bloco DEFLATE a 4096 bytes de saída.
ESP e carregador STM verificam bloco, tamanho, vetores e CRC da aplicação.
Não há alocação dinâmica no carregador. O único bloco de saída é estático.
