# Cardputer ADV Meshtastic 1.0.1

- Iluminação da tela apagada após 30 segundos sem teclado ou mensagens de texto recebidas. Rádio, GPS e persistência continuam ativos.
- Teclas reacendem a tela e executam normalmente sua ação. Mensagens recebidas em qualquer canal ou DM reacendem por outros 30 segundos e emitem um bipe forte de 2 kHz / 320 ms. Telemetria, ACK e TX não acionam o bipe.
- ES8311 a 0 dB e tom de amplitude elevada com envelope de 5 ms. Áudio em tarefa independente com fila limitada, sem aguardar a reprodução no loop do rádio/UI.
- A instrução Enter/detalhes aparece uma única vez no rodapé do chat. Conversas vazias mostram “Sem mensagens nesta conversa”. Digitar na lista não gera outra instrução na área de status.
- Rascunho, cursor, posição de leitura, canais e configuração SD são preservados. Nada altera o schema dos dados.

## Pacote OTA

`cardputer-adv-1.0.1-ota.bin`: 904368 bytes.
SHA-256: `31e87d159ba38c1da92741697e801a6e7bc1539dcf2618c0bbf0fee8153e69ad`.

Aplicação para o M5Launcher já instalado; sem bootloader/partições. Nenhum firmware foi enviado ao aparelho. GPL-3.0 preservada. A release publicada 1.0.0 permanece intacta.

## Validação

Build M8 aprovado. Suíte nativa ASan/UBSan inclui prazo exato de 30 segundos, rollover de millis, despertar por teclado, RX em outro canal e DM, ausência de alerta por ACK, preservação de rascunho/cursor e instrução única com histórico e conversa vazia. Também verifica as regressões existentes de chat contínuo, teclado, SD, checkpoints, backup e OTA.

O usuário confirmou esta versão funcionando no ADV e autorizou a publicação em main. O binário publicado tem os mesmos bytes e SHA-256 da candidata testada. Tamanho/layout/versão do launcher permanecem desconhecidos.

Referência dos registradores/pinos do codec: [M5Unified 0.2.18 oficial](https://github.com/m5stack/M5Unified/blob/0.2.18/src/M5Unified.cpp).

Fontes completas e bibliotecas resolvidas: `CardputerADVMeshtastic-1.0.1-source.tar.gz`, com checksum em `SOURCE_SHA256SUMS.txt`.
