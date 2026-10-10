# 0.7.0-m7 — primeira entrega de M7: mensagens diretas

NÓS → Enter para detalhe → Enter para DM PKI. Fn+Enter envia. Destinatário, histórico e rascunho independentes no microSD; restauração após reboot. ACK/falha/timeout pelo núcleo. Exige NodeInfo com chave pública; o app não envia DM legada automaticamente. Identidade inclui chaves dos dois nós e preserva rascunhos quando uma chave muda.

Aplicação OTA ESP32-S3: **777184 bytes**, SHA256 `a50b7dd632afb0dcf7fcd769a1a9d3ada9e305d2073d2a74f034b322b4a0542d`. 14 executáveis C++ e 10 testes Python aprovados, builds M7/regressão M6 aprovados; DM no ADV ainda precisa de teste físico. Pacotes anteriores preservados. Nenhuma gravação pelo agente; não inclui bootloader ou partições. M5Launcher confirmado, limite/layout instalado não verificados.

M7 permanece em andamento: busca/favoritos, distância, traceroute e diagnóstico/backup são entregas seguintes. [Guia e testes no ADV](../../docs/M7_USAGE.md), [status técnico](../../docs/M7_STATUS.md).
