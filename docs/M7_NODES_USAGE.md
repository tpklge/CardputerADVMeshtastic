# M7 0.7.1 — Nós, busca e favoritos

## Busca

Na lista da aba NÓS, comece a digitar para abrir uma nova busca. Enter aplica o filtro; Fn+Esc cancela a edição. A busca encontra trechos do nome curto, nome longo ou ID hexadecimal, com ou sem `!`, e ignora maiúsculas/minúsculas ASCII. Acentos precisam corresponder à grafia do nome. Limite: 24 bytes.

Fn+seta direita reabre o filtro aplicado. Para limpar, apague o texto e confirme com Enter. Fn+setas cima/baixo percorre os resultados. O filtro aplicado e o modo de favoritos são restaurados do SD; a edição ainda não confirmada fica em RAM.

## Favoritos

Enter abre o detalhe do nó. **F** adiciona ou remove o favorito; a estrela só muda depois da confirmação de gravação do núcleo no SD. Repetição por tecla mantida não dispara nova alteração. Se houver erro, o estado anterior permanece e a tela informa a falha.

Na lista, **Fn+seta esquerda** alterna todos os nós / somente favoritos. O filtro por nome/ID continua valendo. Nomes de favoritos têm `*` e o cabeçalho indica o modo. Não há ordenação diferente; usa-se a ordem do banco do núcleo. Se a lista ficar vazia, desligue o modo ou limpe a busca.

## Detalhes

Fn+setas cima/baixo alterna cinco páginas:

1. Identidade e último RX; Enter continua abrindo DM PKI.
2. Sinal, hops e bateria/voltagem em cache; alimentação externa não vira porcentagem acima de 100.
3. Coordenadas e idade da posição em cache.
4. Distância aproximada e rumo em graus, tendo o norte geográfico como 0°.
5. Data/hora UTC de recepção da telemetria e tempo decorrido. **É hora de recepção, não da medição.** Telemetria recebida há mais de 15 minutos é indicada como antiga.

Para calcular distância, o ADV precisa de fix GPS com até 2 minutos, hora GPS válida e posição remota com até 15 minutos. Coordenadas inválidas, posição futura/sem data e ausência de fix deixam o resultado indisponível. No mesmo ponto, não há rumo definido. A posição remota pode ser aproximada pela configuração de precisão do outro nó; este recurso não substitui navegação.

A data de telemetria vem do histórico no SD. A reconstrução ocorre em segundo plano depois do boot; pode ser necessário reabrir o detalhe após a leitura. Se o cache do núcleo não corresponde ao registro encontrado, ou não havia hora GPS quando a telemetria chegou, aparece “hora desconhecida”. O app não usa o último RX genérico como data da bateria. O detalhe mantém o retrato do nó da abertura; reabra para ver dados novos.

## Teste no ADV

- Procurar nome e ID; confirmar, cancelar e limpar a busca. Reiniciar com filtro aplicado e conferir a restauração.
- Marcar um nó com F, voltar à lista e filtrar favoritos com Fn+esquerda. Reiniciar e conferir estrela/filtro; remover e conferir novamente.
- Abrir as cinco páginas, verificar bateria externa e ausência de telemetria/posição.
- Com GPS local e posição remota recentes, conferir distância/rumo; sem fix, deve aparecer indisponível.
- Após receber telemetria com hora GPS válida, conferir a página de data e reabrir após reboot.
- Conferir DM, canais, rascunhos e ocultação do menu após 5 segundos.

Aplicação entregue exclusivamente como candidato OTA para o M5Launcher. O agente não grava firmware. Traceroute será a próxima entrega de M7.
