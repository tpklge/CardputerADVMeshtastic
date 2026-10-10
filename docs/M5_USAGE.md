# Uso M5 — conversas, nós e GPS

Firmware: `newversion/m5/cardputer-adv-0.5.0-m5-candidate-ota.bin`. Atualizar pelo M5Launcher utilizado, mantendo o microSD com os dados anteriores. Aplicação OTA; nenhum bootloader ou partição incluído.

## Conversas por canal

Na raiz do Chat, **Fn+Esc** abre a lista de conversas. **Fn+cima/baixo** escolhe o slot e **Enter** abre um canal ativo. Slots livres não podem ser abertos. Se estiver escrevendo ou no detalhe, a primeira Fn+Esc volta à lista de mensagens e a segunda abre conversas. A escolha não depende da aba Canais.

Cada canal conserva seu rascunho, cursor e posição no histórico. A troca aguarda a gravação do envio pendente e é concluída somente após salvar o contexto atual. O cabeçalho `#slot nome` é o destino do texto enviado; Fn+Enter envia nesse canal. Mudança da identidade do canal antes de executar um envio cancela a operação.

`[N]` mostra mensagens recebidas não lidas. `[N+]` indica reconstrução do índice de histórico após boot; pode levar mais tempo em cartões com histórico grande. Abrir a página mais recente marca a conversa como lida. Recepção em outra conversa, composição, páginas antigas e outras abas não deslocam foco ou leitura. Mensagens privadas não são abertas nesta etapa; DM será M7.

## Mensagens

Digite para compor; Backspace apaga; Fn+esquerda/direita move o cursor; Fn+Enter envia. Rascunho é salvo no SD e preservado entre abas/reboots. A confirmação de fila gravada limpa apenas o texto que foi enviado, conservando alterações posteriores. TX de rádio não equivale a entrega confirmada.

Na lista, Fn+cima/baixo seleciona; cima após o início carrega mensagens mais antigas; baixo após o fim volta à página recente. Enter abre o texto completo. No detalhe, Fn+cima/baixo rola; Fn+esquerda/direita alterna texto e dados de hora/sinal/saltos/pacote. Hora desconhecida é indicada se o registro não teve UTC válido do GPS; o offset atual é aplicado à hora mostrada. Nome curto do remetente aparece quando está disponível; caso contrário usa o ID.

## Nós e GPS

Nós mostra nome/ID, idade de RX e SNR. Enter abre três páginas; Fn+cima/baixo alterna identidade/cache, rádio/hops/bateria e posição. Dados ausentes aparecem como desconhecidos. Métricas/posição remotas são cache recebido; a idade de posição é calculada quando existe timestamp compatível. RX neste boot usa tempo monotônico; cache anterior depende de UTC válido para calcular idade.

Em Ajustes → GPS/hora, Fn+cima/baixo alterna hora UTC/local/fix/satélites e coordenadas/altitude/idade do fix. Sem fix, coordenadas ficam desconhecidas; a hora GPS pode continuar válida. Fuso permanece em minutos relativos a UTC; Cuiabá: -240. Sem Wi-Fi ou ajuste manual de hora.

## Navegação e conferência

Fonte maior e menu inferior de M4 permanecem. Tab oculto abre o menu sem trocar; Tab visível avança; Shift+Tab volta. O menu expira em 5 segundos desde abrir/trocar, independentemente de digitação ou RX.

No ADV, conferir dados anteriores de M4, seleção de conversa, cabeçalho do destino, rascunho/cursor após reboot, envio/recepção, detalhes, não lidas, nós e GPS. Se já houver mais de um canal ativo, escrever rascunhos distintos, alternar e reiniciar para conferir isolamento. Criar/importar canais por link digitado será M6.
