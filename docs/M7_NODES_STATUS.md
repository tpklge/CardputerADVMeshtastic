# M7 — Segunda entrega 0.7.1: detalhes e busca de nós

O usuário confirmou a DM de 0.7.0 no ADV e autorizou o próximo item. Esta entrega implementa o item 2 de M7; traceroute e diagnóstico/backup permanecem pendentes.

## Implementação

- Busca por trecho de nome curto/longo e ID hexadecimal, 24 bytes, comparação ASCII sem diferenciar caixa. Lista filtrada paginada em quatro nós, sem novo banco ou nova aba. Entrada de texto inicia busca; Fn+direita edita o filtro; Enter aplica e Esc cancela.
- F no detalhe alterna o `is_favorite` nativo do núcleo. A mutação usa a fila do loop e `nodeDB->saveToDisk(SEGMENT_NODEDATABASE)`, que publica o checkpoint completo do SD com readback. Falha restaura o valor em RAM e não anuncia sucesso; a UI só altera a estrela depois de receber confirmação. Fn+esquerda filtra favoritos, combinando com a busca.
- Filtro aplicado e modo favoritos nos bytes reservados 346–371 de UiNavigation/schema 1, validados por UTF-8/limites/CRC. Registros anteriores têm campos zerados. Rascunho/DM/canais mantêm suas identidades anteriores.
- Cinco páginas do detalhe: identificação, rádio/bateria, posição, distância/rumo e hora de recepção da telemetria. Menu e fonte preservados.
- Distância geodésica aproximada por haversine e rumo para norte geográfico. Exige fix local com até 120 s, hora GPS e posição remota com até 900 s, sem data futura e coordenadas dentro das faixas. Pontos coincidentes/antípodas não inventam rumo. Coordenadas remotas podem ser aproximadas pelo emissor.
- Índice RAM de até 200 carimbos de telemetria, reconstruído pelo worker ao validar logs existentes em lotes. Usa a hora GPS capturada no registro da recepção, nunca o último RX genérico do nó. Dados sem hora mantêm hora desconhecida; divergência de bateria/voltagem entre cache do núcleo e evento também oculta a data. Recepção com mais de 900 s é marcada antiga. O detalhe fica congelado até reabrir, mas sua idade é recalculada.

## Verificação

17 executáveis C++ com sanitizers e 10 testes Python aprovados. Casos novos incluem busca/nome/ID/favoritos e paginação, distância cardinal/mesmo ponto/dateline/fixes inválidos/expirados, UI real de busca e favorita com sucesso/falha, páginas de distância/telemetria, serialização de filtros e worker SD com reboot em processo novo. Reconstituído carimbo de telemetria após reboot; recepção sem hora GPS permanece sem hora. Testes de DM, canais, menu, corrupção/interrupção do SD e OTA mantidos.

Renderização 240×135 com fonte real inspecionada nas páginas de distância/telemetria. Build M7 aprovado: aplicação **788.864 bytes**, RAM estática **101.888 bytes**, flash vinculada **788.429 bytes**. Build de regressão M6 aprovado sem reempacotar versões entregues. Base fixada/dependências verificadas; vendor sem modificações. A persistência de favorito pelo núcleo e a interoperabilidade/latência no ADV ainda requerem teste físico desta revisão; testes nativos usam fakes de rádio/banco de nós.

[Aplicação OTA](../newversion/m7-0.7.1/cardputer-adv-0.7.1-m7-candidate-ota.bin). SHA256 `809153df1439808a8299858f9565bb7c0eeac0ef9365b2ba0f8e6a85000c1b4a`. Manifesto/hash das fontes conferidos. Pacotes M1–M6 e M7 0.7.0 preservados com seus checksums verificados. Não inclui bootloader/partições e o agente não enviou firmware. Compatibilidade de instalação e validação física de 0.7.1 continuam não verificadas no manifesto.

[Guia de operação e teste](M7_NODES_USAGE.md). Busca em edição não confirmada fica em RAM; o filtro confirmado fica no SD. Índice de datas pode precisar terminar a leitura após boot; dados muito antigos fora da retenção do histórico ficam sem data conhecida.

## Retorno no ADV

Em 10/10/2026, o usuário confirmou “tudo funcionando” e solicitou ajustes antes de M8. A revisão 0.7.1 foi aprovada no aparelho. A interface atual está em [0.7.2](M7_UI_STATUS.md); a operação descrita acima registra a entrega 0.7.1.
