# Protótipo Wi-Fi antigo (fonte sanitizada)

Esta é a fonte do protótipo de servidor web testado antes de migrarmos para USB OTG. A fonte exige `credentials.h`, que não está incluído. Se quiser reconstruir esse protótipo, copie `credentials.example.h` para `credentials.h` e preencha suas credenciais localmente. Não use esse firmware junto com o USB atual e não compartilhe o arquivo preenchido.

O binário antigo não foi copiado, porque foi compilado com credenciais de rede embutidas. O firmware recomendado e pronto para uso está em `..\release`; ele não inicializa Wi-Fi.
