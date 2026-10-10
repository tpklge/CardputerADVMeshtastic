# Operação da versão 1.0.1

A versão atual é **1.0.1**, confirmada funcionando no ADV pelo usuário. A interface usa quatro abas e mantém os controles validados em 0.7.2. Tab abre o menu e, quando visível, muda de aba; Shift+Tab volta. O menu e a barra superior começam ocultos e aparecem com Tab. Ambos desaparecem cinco segundos após abrir/trocar a aba; digitação/RX não prolongam o prazo. A barra superior não tem fundo destacado: mantém RF/SD/hora à esquerda e apenas a porcentagem estimada de bateria à direita. Usa a estimativa já calculada pelo núcleo a partir da tensão; sem leitura válida mostra `--%`. Abrir uma DM com o menu oculto não o reabre automaticamente. Setas e Esc funcionam sem Fn. Fn nas teclas compartilhadas permite digitar pontuação e links. Enter abre o editor de mensagens; Fn+Enter envia somente com o editor aberto; Esc conserva o rascunho. Durante a composição, o histórico permanece no centro e o texto em edição aparece somente na linha inferior, sem prévia duplicada. Seta direita abre uma mensagem da lista.

## Tela e alerta de mensagens

A iluminação apaga após 30 segundos sem uso do teclado e sem novas mensagens. Qualquer tecla de ação reacende a tela e continua executando sua função normal. Mensagens de texto recebidas em qualquer canal ou DM reacendem a tela por mais 30 segundos e emitem um bipe forte de aproximadamente 320 ms. GPS, telemetria, ACK e mensagens enviadas não acionam o alerta. A rádio e a persistência no microSD continuam funcionando com a tela apagada; isso não é suspensão do ESP32. O bipe usa o ES8311/I2S numa tarefa separada, com fila limitada para rajadas. Novas recepções preservam o rascunho e a posição de leitura do histórico.

A instrução para Enter/detalhes fica apenas no rodapé da lista, inclusive numa conversa vazia.

## Histórico e rascunhos

As mensagens aparecem em ordem cronológica: as recentes ficam embaixo. Cima/baixo percorrem uma única lista contínua, uma mensagem por vez, sem saltos entre páginas, inclusive durante a composição; esquerda/direita continuam movendo o cursor no editor. Na lista, direita abre o detalhe da mensagem selecionada. Setas e Esc não exigem Fn.

Ao ler mensagens antigas, uma recepção não desloca o histórico. Ao voltar à mensagem mais recente, o chat acompanha novas mensagens novamente. Na composição, o histórico acompanha a recepção quando está no final, preservando o texto e a posição do cursor. Quebras de linha/tabulação recebidas são mantidas no histórico e apresentadas no detalhe; a linha resumida usa espaços.

Rascunhos são gravados após um segundo sem alterações, ou após cinco segundos de alterações contínuas, sujeito ao tempo de processamento do cartão. Confirmações, saída do editor e troca de abas solicitam gravação imediata das alterações pendentes. Repetir uma ação que não altera o estado não cria outra geração. O mesmo prazo se aplica ao link em edição.

O log tem **96 segmentos de 512 registros**, até **49.152 registros** (aproximadamente 18 MiB de dados). Esse total inclui mensagens, estados de envio, posições, telemetria e sessões. Ao alcançar o limite, o segmento mais antigo é substituído; não são 49.152 mensagens de texto. A tela Cartão SD informa esse limite.

LVGL mantém seus objetos e altera texto, cor, posição ou visibilidade apenas quando esses valores mudam. Uma composição de tela idêntica não envia outro redesenho ao display. Trocas de aba, menu, seleção, tema e conteúdo continuam atualizando as regiões afetadas.

## Temas

Em **AJUSTES → Tema de cores**, use cima/baixo para ver Escuro, Noite âmbar, Claro ou Preto alto contraste. A prévia aplica o fundo, texto, seleção e menu a toda a tela; há exemplos de envio e recepção. Enter confirma e solicita gravação imediata no microSD; Esc cancela. Tab mantém essa subtela até confirmar/cancelar, e seu menu continua com o timeout normal. Se o SD falhar, a interface informa o erro e não confirma uma preferência nova.

O tema é global: mudar de canal/DM mantém a preferência. Reiniciar recupera o tema da última gravação válida. Os rótulos EU/remetente continuam distinguindo a direção das mensagens. Nos temas Claro e Noite âmbar, as tonalidades de envio e recepção se adaptam ao fundo.

## Traceroute

Em **NÓS**, Enter abre o detalhe do nó e **T** consulta a rota. A consulta usa o módulo oficial Meshtastic: uma em andamento, intervalo mínimo de 30 segundos entre partidas e timeout de aproximadamente 10 segundos. O próprio núcleo mantém ACK/retries; timeout não comprova ausência do nó. O destino precisa ser outro nó conhecido, com rádio/região prontos.

A tela mostra os caminhos e SNR que o núcleo informou. Cima/baixo rola o resultado; Esc volta. O resultado permanece disponível durante a sessão, mesmo depois do prazo de exibição da UI upstream. Um resultado acima de 1.023 bytes aparece como truncado, sem cortar um caractere UTF-8 pela metade. **E** salva o resultado em `/CardputerADVMeshtastic/exports/traceroute-NNN.txt`, com identificação do destino e indicação de truncamento. A mensagem no rodapé confirma sucesso/falha. Não há consultas automáticas.

## Diagnóstico e backup

Em **AJUSTES → Backup / exportar**, use setas e Enter:

- **Exportar diagnóstico:** cria `/CardputerADVMeshtastic/exports/diagnostic-NNN.txt`, com versão/core/schema, estado do rádio/SD/GPS, região/preset/hops/slot, contadores, heap livre/mínimo e maior duração do loop. Não exporta PSKs, chaves, URLs de canais, mensagens, nomes ou coordenadas. Os arquivos anteriores são preservados.
- **Criar backup:** a confirmação informa que inclui identidade e chaves. Enter copia e verifica o último checkpoint completo de configuração para `/CardputerADVMeshtastic/backups/config-NNN/`. São cinco arquivos: identidade, configuração de rádio/segurança, módulos, canais e nós. O manifesto contém versão, revisão do núcleo, ID local, tamanhos e CRCs. Esc cancela antes da operação.
- **Restaurar backup:** escolha um `config-NNN` com as setas, Enter abre a confirmação e outro Enter restaura e reinicia. Esc cancela. O firmware exige o mesmo ID local, revisão do núcleo e versão de configuração, além de validar tamanhos, CRCs e protobufs. Arquivos incompletos/incompatíveis não substituem o checkpoint ativo. Restaurar aguarda a gravação do rascunho e bloqueia enquanto há envio/configuração/consulta pendentes na interface.

O backup é de **configuração**. Histórico, rascunhos, URLs em edição e tema permanecem nos seus arquivos originais no cartão; eles não são copiados nem substituídos pelo backup. Ao restaurar canais/chaves anteriores, as conversas voltam a usar suas identidades correspondentes; outros históricos continuam preservados.

Se o microSD montar, mas a configuração impedir a inicialização do núcleo, a aplicação abre **Backup / exportar**, com **Restaurar backup** selecionado. A restauração funciona sem inicializar rádio ou histórico. Além dos CRCs e versões, compara a identidade do backup com o MAC de fábrica do ADV e o ID armazenado. O backup precisa pertencer ao aparelho. Cartão ausente ou formato de dados de uma versão futura não permite essa recuperação.

Os checkpoints continuam publicando o manifesto somente depois de verificar os cinco arquivos. Arquivos da cópia de destino que já coincidam em tamanho e CRC são reutilizados; apenas os demais são copiados. Falhas anteriores não autorizam reutilizar um arquivo sem verificá-lo novamente. O checkpoint anterior permanece disponível durante a gravação.

As cópias acontecem em worker, sob o lock de checkpoint e transações curtas no SPI compartilhado. Durante a operação, a interface impede outra manutenção. Pode haver espera em uma gravação do núcleo até terminar a cópia. O tempo da operação aparece ao concluir; o diagnóstico permite conferir o impacto no loop do aparelho. Há 64 nomes de backup e 1.000 de cada exportação; nomes de cópias interrompidas também são reservados, sem reutilização automática. Não há formatação ou exclusão automática para liberar espaço.

## Conferência no ADV

1. Troque e cancele temas; confirme um, mude de conversa e reinicie. Confira texto, envio/recepção e aba ativa nas quatro opções.
2. Confirme envio/recepção em canal e DM, rascunho após reboot e menu com cinco segundos.
3. Consulte outro nó com T; confira resposta ou timeout, intervalo entre consultas, rolagem e exportação com E.
4. Exporte diagnóstico. Crie backup, altere um nome/hops e restaure: confira reinício e configuração anterior, mantendo mensagens/rascunhos/tema.
5. Durante a composição, confira cima/baixo no histórico e a chegada de uma mensagem sem alteração do rascunho/cursor. Teste também uma mensagem com várias linhas e a restauração pelo modo de recuperação usando uma cópia do cartão.
6. Confira heap e loop máximo após essas operações. A validação física de 0.7.2 é anterior a essas novas funções.

A entrega é a publicação 1.0.0 da aplicação OTA para o M5Launcher já instalado. O manifesto distingue a confirmação física de 0.8.2 dos testes automatizados da alteração final em 1.0.0. O agente não executou gravação, upload, monitor serial ou atualização de partições.
