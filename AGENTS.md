# Diretrizes deste projeto

- Publicação autorizada em 10/10/2026: versão 1.0.0 no branch main de https://github.com/tpklge/CardputerADVMeshtastic, com histórico contínuo, binários OTA e licença GPL-3.0 existente.
- Etapa autorizada: desenvolvimento e consolidação de M0–M8, incluindo temas e as correções autorizadas de recuperação, resultados de comandos, rascunhos, interface, histórico, checkpoints e ajustes menores, conforme pedido do usuário em 10/10/2026.
- Hardware alvo único: Cardputer ADV, Cap LoRa-1262 oficial SX1262 + GPS e microSD.
- O Cap não é um rádio Meshtastic autônomo conectado por UART: o núcleo de rádio/rede deve executar no ESP32-S3 do ADV. UART do Cap é para GPS.
- Não adicionar BLE, Wi-Fi, TCP, rádio Grove externo, câmera, MQTT ou controle de brilho à aplicação.
- Não depender de celular, computador, SNTP ou internet para uso normal.
- Dados da aplicação e configuração operacional devem ser persistidos no microSD. NVS técnico do ESP32/launcher não é banco de dados deste app.
- Menu inferior: esconder 5.000 ms depois de abrir ou de efetivamente trocar de aba. Digitação, rolagem, RX de rádio e ticks não renovam o prazo.
- Distribuição exclusivamente por OTA no instalador já existente. Compilar/empacotar é diferente de enviar: não executar upload, monitor serial, erase, flash ou alterar bootloader/tabela de partições sem novo escopo explícito do usuário.
- Nenhum script de build poderá aceitar porta serial ou executar gravação opcional.
- M5Launcher foi confirmado pelo usuário; versão/layout/tamanho do slot instalado ainda não foram informados. Build offline de diagnóstico usa layout de referência apenas para linkedição; não modifica a instalação. A publicação de 1.0.0 foi autorizada pelo usuário após confirmação de 0.8.2 no ADV; manter explícitos os limites ainda não verificados do contrato, sem inventar versão, layout ou tamanho do slot. Não impor tabela OTA genérica de dois slots ao aparelho.
- Preservar Tab5Meshtastic e os outros projetos. Consultar `docs/TAB5_ANALYSIS.md` antes de transportar lógica; documentação antiga do Tab5 não comprova implementação.
- LoRa e microSD compartilham SPI: integrar ao lock/transações do núcleo; não inicializar dois drivers independentes nos mesmos sinais.
- Não publicar firmware incompleto nem afirmar validação no hardware sem evidência. Não formatar automaticamente o cartão ou apagar configuração existente na inicialização/migração.

- Interface atual (0.7.2+): Enter abre o editor de mensagens; texto/backspace/envio na lista não alteram o rascunho. Seta direita abre o detalhe de mensagem. Setas/Esc sem Fn; Fn nas teclas compartilhadas digita a pontuação; Fn+Enter envia. Mensagens enviadas verdes e recebidas azuis, com identificação textual. LVGL apenas no loop principal, sem manipulação no worker/ISR.

- Ajuste autorizado após 1.0.0: versão 1.0.1 com iluminação apagada após 30 segundos, despertar por teclado/RX de texto e bipe forte pelo ES8311. Sem suspensão do rádio ou do ESP32; sem configuração de brilho. O usuário confirmou 1.0.1 funcionando e autorizou sua publicação em main, com firmware OTA. Preservar os mesmos bytes do binário testado.
