# Interface 0.7.2 — teclado, editor e cores

Esta é a operação atual. Guias de 0.7.0/0.7.1 que indicam Fn+setas ou Fn+Esc descrevem os controles anteriores.

## Chat

- **Enter** abre o editor, tanto no canal quanto na DM, mesmo quando já há histórico.
- Na lista, digitação, Backspace e Fn+Enter não editam nem enviam o rascunho.
- No editor, digite e use **Fn+Enter** para enviar. Enter sozinho não envia.
- **Esc** sai do editor e preserva o rascunho. Outro Esc abre o seletor de canais.
- **Setas cima/baixo** percorrem o histórico. **Seta direita** abre o detalhe da mensagem selecionada; Esc volta.
- No detalhe, cima/baixo rolam o texto; esquerda/direita alternam texto e metadados.

Mensagens **enviadas são verdes** e **recebidas são azuis**, na lista e no detalhe. O rótulo EU/remetente e o estado de entrega continuam visíveis. Fundo destaca a seleção; cores não substituem a indicação de ACK/erro/timeout.

## Teclas compartilhadas com pontuação

Setas e Esc funcionam **sem Fn** em todas as telas. Nas teclas físicas compartilhadas, Fn permite escrever a pontuação original:

| Tecla sem Fn | Com Fn |
|---|---|
| Esc | acento grave (&#96;) |
| esquerda | vírgula (,) |
| cima | ponto e vírgula (;) |
| baixo | ponto (.) |
| direita | barra (/) |

Shift continua gerando os símbolos impressos de maiúscula/pontuação. Fn+Enter continua sendo envio. Isso permite digitar os links Meshtastic completos, inclusive barras e pontos.

Na aba NÓS, digite para buscar, Enter aplica, direita edita filtro e esquerda filtra favoritos. F no detalhe marca/remove favorito; cima/baixo alterna as cinco páginas. Enter no detalhe abre a DM; na DM, Enter abre o editor.

Tab/Shift+Tab preservam o menu inferior: primeiro abrem se estiver oculto; quando visível, alternam as quatro abas. O menu some 5 segundos depois de abrir ou trocar de aba. Digitação, rolagem e RX não renovam esse prazo.

## Conferência no ADV

1. No Chat, digitar antes de Enter deve preservar o rascunho. Enter deve permitir editar; Fn+Enter envia. Conferir canal e DM.
2. Usar setas e Esc sem Fn nas listas/editor; digitar vírgula/ponto/barra com Fn e testar edição de link.
3. Conferir verdes/azuis e seleção legível, inclusive após ACK e abrindo detalhes.
4. Reiniciar com rascunho e conferir persistência; o editor deve voltar fechado, exigindo Enter.
5. Conferir orientação da tela, caracteres acentuados, menu de 5 s, GPS, favoritos, SD e RX/TX.

A interface agora é desenhada pelo LVGL, com buffer por faixas e fonte Adv_Plain_12 preservada. O pacote é somente aplicação OTA, sem bootloader ou tabela de partições. A revisão anterior permanece disponível. Traceroute e diagnóstico/backup continuam pendentes no roteiro de M7; M8 não foi consolidada nesta entrega.


Atualização de 10/10/2026: 0.7.2 confirmada pelo usuário; temas e ferramentas restantes entregues no candidato [M8](M8_USAGE.md). As pendências descritas acima se referem ao escopo desta entrega anterior.
