# Cardputer ADV Meshtastic 1.0.0

Primeira publicação do cliente standalone para Cardputer ADV com Cap LoRa-1262 oficial, GPS e microSD. Licença **GNU GPL versão 3**, mantendo o LICENSE original do repositório.

O chat agora oferece **uma única lista contínua por canal ou DM**. Mensagens recentes ficam embaixo; cima/baixo avançam uma mensagem por vez, com sobreposição da janela visível, sem saltos entre páginas. O histórico permanece no SD e é consultado conforme a rolagem, sem carregar todos os registros em RAM. Nos extremos, outra seta mantém a posição. Ler mensagens antigas suspende o acompanhamento automático; voltar à mais recente retoma a recepção no final.

Enter abre a composição; Fn+Enter envia. Durante a composição, cima/baixo navegam no histórico e esquerda/direita movem o cursor. Novas mensagens preservam rascunho e cursor. Direita na lista abre o texto completo e os metadados da mensagem selecionada. Setas/Esc não exigem Fn.

Inclui as funções confirmadas até 0.8.2: canais editáveis/importação de links, DM PKI, nós/busca/favoritos/posição, traceroute manual, diagnóstico, backup/restauração e recuperação, quatro temas, interface LVGL com atualização incremental, rascunhos periódicos, resultados de comandos reservados e checkpoints verificados.

## Downloads

- **`cardputer-adv-1.0.0-ota.bin`**: aplicação para o fluxo OTA do M5Launcher já instalado. **884864 bytes**, SHA-256 `99369d35f40a4ec189719dbee1fd6df646a62d19d9d7a50ab08464bc04d9adbb`.
- `SHA256SUMS.txt` e `manifest.json`: checksum da aplicação, versões/commits fixados, hash das fontes e limites de instalação ainda não verificados.
- `CardputerADVMeshtastic-1.0.0-source.tar.gz`: fontes da aplicação, submódulos utilizados e bibliotecas resolvidas no build, com as respectivas licenças/avisos. `SOURCE_SHA256SUMS.txt` verifica esse arquivo.

O pacote contém somente a aplicação; não há bootloader ou tabela de partições. Nenhum firmware foi enviado ao aparelho pelo agente. O binário 0.8.2 confirmado pelo usuário fica preservado em `newversion/m8-0.8.2` no repositório.

## Validação

Build M8 e regressão alvo M6 aprovados. RAM estática: **148016 bytes**; flash vinculada: **884429 bytes**. Suíte nativa com ASan/UBSan e **12 testes Python de OTA** aprovados; LVGL real, teclado e composição aprovados. O teste final percorre 12 mensagens nos dois sentidos, verifica as linhas sobrepostas, os extremos e uma nova recepção enquanto lê mensagens antigas, além de preservar o rascunho/cursor e abrir o detalhe da mensagem correta.

A versão **0.8.2 foi confirmada funcionando no ADV pelo usuário**. A alteração final de rolagem em 1.0.0 foi validada por testes automatizados; o manifesto não afirma teste físico desta revisão. Versão/layout/tamanho do slot do launcher instalado permanecem desconhecidos e explicitamente não verificados.

O log retém até 49.152 registros, incluindo mensagens, estados de entrega, posições, telemetria e sessões; ao atingir o limite, substitui o segmento mais antigo. Esperas de SD antes do TX, restauração normal e backup permanecem; índices de histórico, cache de nós e medições prolongadas de latência são melhorias futuras.

[Operação da aplicação](https://github.com/tpklge/CardputerADVMeshtastic/blob/v1.0.0/docs/M8_USAGE.md) · [Fontes e compilação](https://github.com/tpklge/CardputerADVMeshtastic/tree/v1.0.0)
