# Roteiro de implementação

## Resultado pretendido

Um ADV com LoRaCap oficial que inicializa, configura a rede/canais, envia e recebe mensagens, consulta nós, mostra GPS/hora e recupera seus dados do microSD sem recorrer ao celular. Interface e núcleo Meshtastic pertencem ao mesmo firmware. Atualizações são entregues como aplicação OTA.

Este documento registra o roteiro completo. M0–M8 têm desenvolvimento autorizado. O usuário confirmou as entregas no ADV até 0.7.2, incluindo teclado/editor/LVGL/cores. A versão 0.8.0 conclui os itens restantes de M7 e acrescenta temas persistidos, com consolidação offline de M8 e pacote candidato. A aceitação física das novidades e a verificação do app slot do M5Launcher instalado permanecem pendentes. Evidência e limites em [M8_STATUS.md](M8_STATUS.md); operação em [M8_USAGE.md](M8_USAGE.md). O agente não enviou firmware ao aparelho.

## Ordem dos marcos

| Marco | Entrega | Dependências |
|---|---|---|
| M0 | Base/build/candidato implementados; contrato instalado pendente | M5Launcher confirmado; limite ainda desconhecido |
| M1 | Diagnóstico implementado; validação básica confirmada pelo usuário | Base M0; RX/TX mesh será validado com M2 |
| M2 | Núcleo/adaptador implementados; persistência e RX/TX básicos confirmados | M1 |
| M3 | Snapshots, histórico e rascunhos implementados; persistência confirmada pelo usuário | M1 + M2 |
| M4 | Shell 0.4.0 confirmado pelo usuário | M1; integração final com M2/M3 |
| M5 | Conversas por canal e detalhes implementados; candidato 0.5.0 | M2 + M3 + M4 |
| M6 | Canais e links implementados; candidato 0.6.0 | M2 + M3 + M4 |
| M7 | DM, detalhes e ferramentas adicionais | M5 + M6 |
| M8 | Estabilização e pacote OTA de versão | Marcos necessários ao release |

M4 pode começar com dados simulados após M1, mas não comprova funcionamento do núcleo ou persistência. O **MVP é M0–M6**; M7 amplia a experiência após consolidar os requisitos centrais.

## M0 — Base e instalação

Trabalho:

- Selecionar uma release/commit do firmware oficial com suporte ADV/Cap; fixar revisão, submódulos, protobufs, toolchain e bibliotecas.
- Definir fork upstream com código próprio integrado ou overlay reprodutível. Mapear seu setup/loop, display, teclado, locks SPI e storage.
- Registrar instalador OTA que o usuário já utiliza, tabela vigente, tamanho máximo de aplicação e mecanismo de retorno/recuperação.
- Definir flags para excluir Wi-Fi/Bluetooth/MQTT e substituir a UI original sem desligar GPS ou drivers necessários.
- Preparar build de aplicação e empacotamento sem função de upload; ativar configuração de PlatformIO somente depois dessas decisões.
- Registrar atribuição e licenças das dependências efetivamente incorporadas.

Concluído quando: revisão e dependências estão fixas; existe caminho documentado de build para um binário de aplicação compatível com o instalador; não há alteração exigida de bootloader/partições nem comando de gravação no fluxo.

Ponto de atenção: a revisão develop pesquisada é evidência técnica, não escolha automática para um produto estável. O limite do app vem do instalador, não dos 8 MB totais de flash.

## M1 — Hardware e barramentos

Trabalho:

- Integrar display ST7789, teclado TCA8418 e leitura de bateria do ADV com o suporte da base.
- Validar teclas/modificadores/repetição e mapear eventos lógicos sem depender de botão G0.
- Inicializar Cap/switch RF, SX1262, UART GPS e microSD respeitando CS e locks do SPI compartilhado.
- Fazer leitura/escrita SD em worker com transações curtas; não bloquear callbacks/ISR de rádio.
- Instrumentar erros de SD, ocupação do SPI, memória livre e latência por diagnóstico em tela/arquivo.

Concluído quando: tela/teclado funcionam após boot frio; rádio e SD operam juntos, sem colisão de CS ou perda de recepção atribuída a acesso concorrente; GPS gera dados válidos. Verificação física ocorrerá apenas na etapa de implementação/validação solicitada posteriormente.

## M2 — Núcleo Meshtastic local

Trabalho:

- Manter roteamento, criptografia, deduplicação RF, IDs e retries do núcleo oficial.
- Criar adaptador para enviar texto a canal/nó, consultar páginas de nós e receber texto/posição/telemetria/feedback de TX.
- Copiar dados necessários dos eventos para o estado da aplicação sem reter ponteiros a pacotes liberados pelo núcleo.
- Implementar comandos assíncronos, filas limitadas e erros apresentados à UI.
- Suportar configuração local de nome longo/curto, região, preset e parâmetros essenciais; aplicar pelos caminhos corretos do núcleo.
- Distinguir rádio pronto de erro/Cap ausente, e histórico em cache de nó ouvido neste boot.

Concluído quando: interoperabilidade básica com um nó Meshtastic conhecido comprova RX/TX de texto e dados de nó/posição; nenhum transporte externo é necessário. Ajustes locais notificam e reconfiguram o núcleo corretamente.

## M3 — Persistência no microSD

Trabalho:

- Implementar [STORAGE_SPEC.md](STORAGE_SPEC.md): namespace exclusivo, log versionado, CRC, páginas e snapshots de configuração.
- Persistir identidade/chaves do nó, canais e LoRa pelo adaptador do storage upstream; eliminar concorrência entre cópia em flash e cópia SD como fontes de verdade.
- Persistir histórico completo, estados de envio, nós, preferências, rascunhos e índice de conversas.
- Recuperar último snapshot íntegro e último registro completo; reconstruir índices quando necessário.
- Tratar cartão ausente, cheio, somente leitura, removido e dados incompatíveis; nunca formatar automaticamente.
- Preservar configuração válida nas migrações e nas atualizações OTA; salvar geração anterior para recuperação.

Concluído quando: reinício recupera identidade, configuração e conteúdo; corte durante escrita não produz sucesso falso nem perda de toda a base; sem SD a UI informa estado e não gera silenciosamente identidade nova. RAM permanece limitada durante leitura de histórico extenso.

## M4 — Interface compacta

Implementado em 0.4.0. Editor de texto, listas, detalhes e formulários integrados aos dados reais de M2/M3; infraestrutura reutilizável para URLs em M6. Validação offline em [M4_STATUS.md](M4_STATUS.md); operação em [M4_USAGE.md](M4_USAGE.md).

Trabalho:

- Criar barra superior e quatro abas conforme [UI_SPEC.md](UI_SPEC.md).
- Implementar menu inferior com timeout monotônico de 5.000 ms a partir de abertura/troca real de aba.
- Preservar posição de leitura, seleção e rascunho ao mudar de aba ou ocultar menu.
- Criar componentes de lista, detalhe paginado, formulário de um campo por vez e mensagens curtas de erro/progresso.
- Implementar editor de texto/URL com cursor, rolagem horizontal, limites por bytes e acesso aos símbolos necessários.
- Evitar que tráfego de rádio roube foco ou troque a conversa ativa.

Concluído quando: navegação física é suficiente; timeout funciona nos limites de 4.999/5.000 ms; digitação e eventos não mantêm menu aberto; nenhum formulário extrapola 240×135.

## M5 — Chat, nós e GPS/hora

Implementado em 0.5.0: escolha de canal, histórico/rascunho por conversa, indicadores não lidos, metadados da mensagem, páginas de nós/GPS. Beep opcional não adicionado. Evidência em [M5_STATUS.md](M5_STATUS.md); teclas em [M5_USAGE.md](M5_USAGE.md).

Trabalho:

- Listar conversas de canais ativos com contador de não lidas; escolher destino no cabeçalho/editor.
- Exibir histórico por páginas, abrir mensagem completa e voltar à última posição.
- Composer dedicado com rascunho salvo, contador de bytes e envio explícito; mostrar fila/transmitida/falha sem afirmar entrega inexistente.
- Paginar nós com nome curto/ID, tempo desde último RX e sinal; detalhar nomes, hops, posição e métricas disponíveis.
- Ler hora/posição do GPS através do núcleo, separar validade temporal de fix posicional e permitir apenas offset local em Ajustes.
- Alertar RX com indicador e beep opcional simples, sem mudança automática de aba.

Concluído quando: fluxo completo de escrever/enviar/receber/reiniciar conserva mensagens e rascunhos; canal de saída coincide com o destino mostrado; UTF-8 não é truncado; nós e hora distinguem desconhecido/desatualizado de valor válido.

## M6 — Canais standalone

Implementado em 0.6.0. [M6_STATUS.md](M6_STATUS.md) registra evidências/limites e [M6_USAGE.md](M6_USAGE.md), operação no teclado. Leitura de `.url` é conveniência futura; digitação/exportação estão implementadas.

Trabalho:

- Listar slots, criar secundário, editar nome/chave e desativar secundário.
- Gerar PSK AES-256 usando RNG do ESP32; também validar chave digitada/Base64 e formatos aceitos pelo núcleo.
- Portar/alinhar codec de URL do Tab5 e validar com fixtures do protocolo escolhido.
- Permitir digitar **link completo**, editar antes de decodificar e visualizar revisão paginada.
- Implementar adicionar ou substituir conjunto, preflight de slots/duplicatas/chaves herdadas e revisão de LoRa quando presente.
- Salvar e validar uma geração SD antes de aplicar; mostrar erro real se persistência ou reconfiguração falhar.
- Exportar URL por rolagem e arquivo `.url` no cartão. Leitura de `.url` no SD é conveniência complementar, nunca substitui digitação.

Concluído quando: o usuário cria canal e importa um conjunto sem ferramentas externas; links inválidos/sem espaço não alteram nada; resultado sobrevive ao reinício; um nó compatível comunica nos canais importados. Este marco fecha o MVP.

## M7 — Ampliações contidas

Os quatro grupos estão implementados. DM, busca/favoritos e ajustes de UI foram confirmados pelo usuário até 0.7.2. Traceroute e diagnóstico/backup/restauração são entregues em 0.8.0; ver [M8](M8_STATUS.md).

Ordem recomendada:

1. **DM:** iniciar pelo detalhe do nó, distinguir conversa privada, persistir destinatário e receber corretamente após reinício. Integrar ACK/erro/timeout com correlação por packet ID, sem criar retransmissor concorrente. Testar modalidade de segurança/PKI suportada pela base.
2. **Detalhes adicionais:** bateria/telemetria com data e validade, distância/rumo simples quando ambos os fixes são válidos, favorito de nó e busca por nome/ID.
3. **Traceroute manual:** uma consulta por vez, limite de frequência, resultado paginado e timeout, usando implementação do núcleo. É recurso novo em relação ao Tab5 analisado.
4. **Diagnóstico/backup:** exportar diagnóstico sem PSKs e escolher/restaurar backup compatível pelo teclado.

Cada ampliação deve caber em subtela; não criar novas abas. Mapas, QR de conjuntos longos, áudio de voz e painel extensivo de telemetria permanecem fora deste roteiro inicial.

## M8 — Release e OTA

Consolidação offline e candidato 0.8.0 implementados. Homologação física/contrato do slot instalado seguem como critérios de aceitação abertos, sem impedir o pacote candidato autorizado. Temas de alto contraste acrescentados conforme o pedido do usuário.

Trabalho:

- Executar [matriz de validação](VALIDATION_PLAN.md), medir RAM/latência/tamanho e corrigir gargalos.
- Preparar versão que cumpra [OTA_CONTRACT.md](OTA_CONTRACT.md), incluindo migrações de dados reversíveis/compatíveis.
- Gerar exclusivamente aplicação `.bin`, SHA-256 e manifesto com alvo, revisão, tamanho, schema e contrato de instalação.
- Documentar limitações ainda observadas e preservar último pacote de versão conhecido.

Concluído quando: os critérios obrigatórios passam, o binário cabe no app slot aceito e o pacote está pronto para o fluxo OTA existente. Compilar e preparar o pacote não implica enviar ao ADV nesta etapa.

## Revisão 0.8.2 — Correções autorizadas

Implementados os itens 1, 2 e 4 da análise para 1.0: restauração de backup sem núcleo/histórico prontos, resultados de comandos com capacidade reservada e prazo máximo de gravação do rascunho, evitando gerações idênticas. Acrescentados atualização incremental LVGL, histórico cronológico e paginação nos dois sentidos, recepção durante composição, reutilização verificada de arquivos em checkpoints e os ajustes de texto multiline/teclas/retenção.

A revisão é candidata a teste físico. A remoção das esperas de SD no loop principal (item 3 da análise) continua pendente; a gravação antes do TX, o flush antes da restauração normal e o lock durante backup mantêm o comportamento atual. Indexação do histórico, cache de nós, ensaios prolongados e contrato real do launcher continuam no roteiro para 1.0.

## Decisões para o início da implementação

M0 selecionou a base Beta `v2.7.26.54e0d8d` e o usuário confirmou M5Launcher. Versão/layout/tamanho do slot instalado e mecanismo de retorno/recuperação ainda precisam de evidência. O candidato M1 é compilado offline sem afirmar esses dados nem modificar partições.

## Publicação 1.0.0

Autorizada pelo usuário em 10/10/2026 após confirmar 0.8.2 funcionando. A alteração final mantém um único histórico contínuo por conversa: setas movem a seleção uma mensagem por vez, com janelas sobrepostas consultadas no SD. Publicação no branch `main`, tag `v1.0.0` e binários OTA na release, preservando a licença GPL-3.0 existente. A confirmação física registrada é de 0.8.2; slot/layout do launcher permanecem não verificados. Melhorias futuras de latência/índices não são apresentadas como implementadas.
