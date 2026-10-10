# M7 — Primeira entrega 0.7.0: DM PKI

M6 foi confirmado pelo usuário; M7 foi autorizado. Esta entrega implementa o primeiro item de M7. O marco completo permanece em andamento: detalhes adicionais/busca/favoritos, traceroute e diagnóstico/backup ainda não foram implementados.

## Implementação

- Enter no detalhe do nó abre conversa privada no Chat; indicação de destinatário e PKI. Nós sem contexto PKI exibem orientação sem criar envio. Nenhuma aba adicional.
- Destinatário/cursor/rascunho e histórico privados persistem no microSD, usando pares CRC por identidade e salvando o contexto anterior antes da troca. Campo reservado 342–345 no registro schema 1 armazena o destinatário; registros anteriores têm zero e mantêm conversas de canal.
- Identidade privada inclui IDs e chaves públicas dos dois nós. Não depende do nome/PSK/slot do canal; identidades de grupos não mudaram. Mudança de chave bloqueia envio com contexto antigo e preserva o arquivo antigo; reabrir o nó inicia/carrega o contexto da chave atual.
- Pacote solicita explicitamente PKI, fixa a chave pública esperada e pede ACK. O Router da revisão fixada verifica a chave do destinatário antes de cifrar, sem fallback legado. A fila exige gravação verificada no SD antes de transmitir; retries/timeout ficam no núcleo.
- RX privado destinado ao ADV e decifrado com PKI é persistido por remetente. RX de outro contexto avisa sem alterar foco/texto. Mensagens legadas sem PKI não são incorporadas à conversa protegida. Modalidade licenciada, nós sem chave e o próprio ADV não habilitam DM.
- Histórico correlaciona feedback por packet ID e sessão, distinguindo fila, TX radio, ACK, NAK/falha e timeout. Resultado desconhecido após reboot não é apresentado como confirmação.

## Validação

14 executáveis C++ com AddressSanitizer/UndefinedBehaviorSanitizer, mais 10 testes Python, aprovados. Novos casos exercitam UI real: abertura privada, bloqueio sem PKI, destinatário/identidade no comando, retorno ao canal e restauração de UI privada. Worker SD real com fakes: rascunhos independentes, destinatário persistido, RX privado filtrado, ACK/timeout por packet ID e reboot em processo novo. Mantém os casos anteriores de CRC/corrupção/interrupção, teclado, canais e menu 5 s.

Build M7 aprovado: aplicação **777.184 bytes**, RAM estática **96.360 bytes**, flash vinculada **776.753 bytes**. Build de regressão M6 aprovado sem reempacotar versões anteriores. Fontes upstream fixadas e dependências Python verificadas; checkout vendor não modificado. Testes nativos não executam o rádio/cripto do ESP32: interoperabilidade PKI e ACK no ADV exigem o teste físico do usuário.

[Aplicação OTA](../newversion/m7/cardputer-adv-0.7.0-m7-candidate-ota.bin). SHA256 `a50b7dd632afb0dcf7fcd769a1a9d3ada9e305d2073d2a74f034b322b4a0542d`. Manifesto registra fontes/inventário e mantém validação física/compatibilidade de instalação como não verificadas. Nenhuma gravação, bootloader ou tabela de partições incluídos; M1–M6 preservados.

## Limites desta entrega

Abertura/reabertura de DM ocorre pelo detalhe do nó. Não há lista independente de DMs nem contador persistente de não lidas por nó; o índice de badges dos canais continua limitado a oito slots. O histórico anterior à adoção de PKI não é migrado para a nova identidade. Menu/fonte/GPS/canais mantêm o comportamento de M6. [Guia e conferência no ADV](M7_USAGE.md).

## Retorno e continuação

O usuário confirmou “funcionou, proxima etapa” após a entrega 0.7.0. DM foi aprovada no ADV. A segunda entrega 0.7.1 implementa o item 2: veja [detalhes dos nós](M7_NODES_STATUS.md). As descrições acima registram o escopo da primeira entrega; traceroute e diagnóstico/backup continuam pendentes.
