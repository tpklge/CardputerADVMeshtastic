# 0.2.1-m2 — correção da persistência no SD

Corrige a substituição de arquivos existentes em FAT: guarda a geração anterior em `.bak`, promove o temporário verificado e somente então remove a cópia anterior. Se a promoção falhar, tenta restaurar o original.

No boot, valida protobuf e versão 24 dos quatro arquivos de configuração. Recupera arquivo ausente de um temporário válido ou backup válido. Quando o original está válido, preserva temporários e backups remanescentes em nomes únicos `.saved-N`. Não formata o cartão nem redefine região, hops ou canais. Configuração incompleta sem cópia válida continua bloqueando o núcleo.

Correção motivada pelo relato de SD erro/configuração incompleta após configurar BR_902 e hops 3. Após instalar 0.2.1, o usuário confirmou configuração persistente, mudança para ANZ e envio/recepção de mensagens no ADV. O conteúdo original do cartão não foi inspecionado; a causa exata da falha anterior permanece inferida. Testes locais incluem substituição em FAT e falhas antes/durante a promoção. Distribuição apenas como aplicação OTA pelo launcher existente.

# 0.2.0-m2 — candidato de integração

Núcleo Meshtastic local com ReliableRouter, criptografia e drivers oficiais de SX1262/GPS. Teclado e tela provisória de status/nós/ajuda; configuração local de nome/região/preset/slot/hops e envio de texto por Fn+Enter.

Configuração básica do núcleo em `/CardputerADVMeshtastic/core`, somente no microSD. Sem cartão gravável, sem Cap ou com configuração incompleta/incompatível, o núcleo não inicia. Sem formato automático, Wi-Fi, BLE ou transporte externo. Região inicialmente UNSET; escolha local libera TX.

Build e testes offline aprovados; boot M2 e interoperabilidade RF aguardam validação no ADV. Histórico/canais/UI final/recuperação de snapshots pertencem aos próximos marcos. Manifesto informa runtime mesh habilitado e hardware/compatibilidade instalada ainda não verificados.

Operação: [M2_USAGE.md](../../docs/M2_USAGE.md). Detalhes: [M2_STATUS.md](../../docs/M2_STATUS.md). Pacote contém somente aplicação OTA, checksum e manifesto; nenhum firmware enviado pelo agente.
