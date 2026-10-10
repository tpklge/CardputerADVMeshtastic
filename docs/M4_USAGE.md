# Uso M4 — quatro abas

Aplicação OTA: `newversion/m4/cardputer-adv-0.4.0-m4-candidate-ota.bin`. Atualizar pelo M5Launcher já utilizado, mantendo o microSD com os dados de M3. O pacote contém somente a aplicação.

## Navegação

- **Tab**: com menu oculto, abre sem mudar a aba; com menu visível, vai à próxima aba.
- **Shift+Tab**: mesma abertura, depois volta à aba anterior.
- O menu some 5 segundos após abrir ou trocar efetivamente a aba. Digitar, rolar e receber mensagens não renovam o prazo.
- **Fn+setas**: seleção/rolagem; esquerda/direita movem cursor no editor.
- **Enter**: abre detalhe ou ajuste; confirma um campo.
- **Fn+Esc**: volta/cancela; na raiz, fecha o menu.
- **Fn+Enter**: envia o rascunho somente no Chat.

A fonte maior de M3 foi mantida. Trocar abas preserva rascunho, cursor, seleção, posição de leitura e formulário aberto durante a sessão. No reboot, restaura a aba raiz e suas seleções, rascunho e cursor; formulários e diálogos não são reabertos.

## Chat

Mostra mensagens do canal primário. Digite para compor, use Backspace e Fn+esquerda/direita para editar; Fn+Enter envia. Esc volta à lista sem apagar o texto. Na lista, Enter abre a mensagem completa e Fn+cima/baixo percorrem o texto. Na lista, cima após o início carrega a página anterior; baixo após o fim volta à página mais recente.

RX mostra um indicador sem mudar a aba, o foco ou uma leitura em andamento. O texto só é limpo após confirmação da fila local gravada; se você já tiver alterado o rascunho enquanto o envio estava pendente, o novo texto é conservado. TX de rádio e ACK são estados distintos.

## Nós e Canais

Nós lista nome curto/ID, com seleção e páginas de quatro entradas. Enter abre detalhe; Fn+cima/baixo alternam páginas de nome/cache e sinal/posição. Canais mostra oito slots, nome, papel e estado de criptografia. É consulta nesta etapa; criar, editar e digitar links de importação será M6.

## Ajustes

Nome curto/longo, região, preset, hops, slot e offset UTC são editados em um campo por vez. Enter salva; Esc cancela. Região/preset podem ser escolhidos com Fn+cima/baixo ou digitados pelo nome exato. Fuso usa minutos em relação a UTC, de -720 a +840; Cuiabá corresponde a -240. Hora vem do GPS, independente do fix posicional.

GPS/hora, cartão SD, diagnóstico e ajuda têm telas próprias. Limpar rascunho exige Enter em uma confirmação; Esc cancela. Enquanto essa confirmação está aberta, troca de aba fica bloqueada, mas o menu continua expirando.

## Conferência no ADV

Confirmar quatro abas e fonte; observar o menu sumir enquanto digita ou recebe; alternar abas com rascunho/formulário aberto; enviar/receber e abrir detalhes; consultar nós/canais; salvar fuso e reiniciar para conferir aba, seleção e rascunho. Os dados existentes de M3 devem permanecer disponíveis.
