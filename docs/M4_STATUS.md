# M4 — interface compacta, candidato 0.4.0

M3 foi confirmado pelo usuário, incluindo persistência do rascunho após reboot. M4 implementado em 09/10/2026; testes e compilação offline aprovados. O usuário confirmou funcionamento da etapa M4 antes de autorizar M5. Nenhum firmware enviado ou aparelho acessado pelo agente.

## Implementação

- Quatro abas fixas CHAT/NÓS/CANAIS/AJUSTES, com barra de RF, SD, hora GPS com offset e tensão da bateria.
- Menu inferior de 18 px, quatro áreas de 60 px. Expira em 5.000 ms desde abertura/troca real; digitação, RX e rolagem não renovam. Tab oculto abre sem trocar; Tab visível avança e Shift+Tab retrocede. Diálogo de limpeza bloqueia troca, mantendo timeout ativo.
- Fonte nominal 12 de M3, linhas de 16 px e formulário de um campo por vez. Renderização 240×135 inspecionada com a própria fonte em framebuffer nativo.
- Editor reutilizável com cursor UTF-8, rolagem horizontal e limites em bytes. Rascunho e cursor seguem persistidos no SD. Durante a sessão, abas conservam formulários/subtelas e posições; após reboot, restaura a raiz da aba salva, seleções e rascunho.
- Lista/histórico do canal primário, detalhe rolável e compositor conectados ao worker de M3. RX não rouba foco nem muda aba. Confirmação assíncrona de fila não apaga texto alterado após solicitar envio.
- Nós: lista/páginas, seleção e detalhe de cache/sinal/posição disponível. Canais: oito slots em consulta, papel e criptografia sem revelar chaves.
- Ajustes: formulários locais de identidade/região/preset/hops/slot/fuso; GPS/hora, SD, diagnóstico, ajuda e confirmação de limpeza do rascunho.
- Codec SD usa campos reservados de registros M3, mantendo schema 1/CRC e os dados existentes. M3 sem marcador é interpretado como UI antiga. Núcleo, snapshots e política de persistência permanecem os mesmos.

M5 continua responsável por consolidar conversas/canais ativos, indicadores e detalhes mais completos. Criar/editar/importar links de canais é M6. O editor atual prepara a infraestrutura; não declara importação implementada.

## Verificação

Seis binários C++ com ASan/UBSan: UI integrada, menu/editor, codec de storage/snapshots, substituição de arquivos, teclado e validação mesh. Sete testes Python de imagem/empacotamento OTA. Limites 4.999/5.000 ms, rollover monotônico, RX durante timeout, troca de abas mantendo rascunho/formulário, confirmação bloqueada e TX com edição posterior cobertos. Frames nativos gerados em `.pio/ui-preview`.

Build M4: aplicação ESP32-S3 **754.112 bytes**, RAM estática **85.960 bytes**, flash vinculada **753.677 bytes**. Pacote inspecionado por checksum de segmentos e SHA embarcado. Build de regressão M3 também executado; pacotes anteriores preservados.

Candidato: [aplicação OTA](../newversion/m4/cardputer-adv-0.4.0-m4-candidate-ota.bin). SHA256: `8c4d67e95e13cd8aa19495f9650233cf9d7a866a6237df6ee828fc17d33b2f70`. Upstream continua `v2.7.26.54e0d8d`. Manifesto contém versões, inventário e hash das fontes. M5Launcher confirmado; versão/layout/limite instalado continuam desconhecidos, portanto o manifesto mantém compatibilidade instalada não verificada.

Conferência física e teclas em [M4_USAGE.md](M4_USAGE.md). Testar navegação, fonte, timeout durante digitação/RX, persistência após reboot e dados anteriores de M3.
