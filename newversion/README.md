# Aplicações OTA

`m1/` conserva diagnóstico 0.1.1; `m2/`, candidato 0.2.1 com configuração persistente e RX/TX confirmados; `m3/`, 0.3.0 com funcionamento e rascunho após reboot confirmados pelo usuário. `m4/` conserva 0.4.0 confirmado pelo usuário. `m5/` contém candidato 0.5.0 com conversas por canal e detalhes, aguardando teste no ADV. `m6/` contém 0.6.0 com canais editáveis/importação digitada/exportação SD, aguardando teste. Os pacotes anteriores foram preservados.

Cada pacote tem aplicação `.bin`, checksum, manifesto e notas. Nenhum firmware foi enviado pelo agente. Limite do slot instalado ainda desconhecido; bootloader e partições não compõem os pacotes. Ver [contrato](../docs/OTA_CONTRACT.md) e [uso M6](../docs/M6_USAGE.md).
