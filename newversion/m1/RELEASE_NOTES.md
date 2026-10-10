# 0.1.1-m1 — orientação da tela

Corrige a tela invertida com rotação de 180° após inicializar o ST7789. Usuário confirmou boot e leitura básica do teclado na versão 0.1.0-m1; na 0.1.1-m1 confirmou orientação corrigida, teste de microSD, hora GPS e Cap OK. Relatou RX zerado e zero erros; TX está desabilitado. Fix de posição, recepção RF e coexistência RX/SD aguardam validação física.

Primeiro candidato compilado para Cardputer ADV + Cap LoRa-1262 + microSD. Aplicação OTA somente, sem firmware merged/bootloader/partições.

- Tela local ST7789, teclado/combinações/repetição e bateria em volts.
- GPS UTC/fix/NMEA; Cap e SX1262 com RX bruto de bancada 915 MHz.
- Enter executa teste SD de 4 KiB com readback e relatório; SPI compartilhado com lock upstream e métricas.
- Startup recusa autoerase de NVS técnico do launcher.
- Sem TX, runtime mesh, chat ou configuração de canais neste estágio.

Build e testes nativos da base passaram. Instalador confirmado: M5Launcher; versão/layout/limite de slot instalado pendentes. Detalhes em `docs/HARDWARE_BRINGUP.md` e `docs/M0_M1_STATUS.md`.
