# Persistência no microSD

## Fonte de verdade

microSD é o armazenamento persistente do produto: preferências, identidade e chaves do nó, LoRa, canais, histórico, estados de envio, rascunhos e nós conhecidos. Flash/NVS continuam disponíveis para necessidades técnicas do ESP32/instalador, mas não serão backend alternativo silencioso para esses dados.

No upstream pesquisado o ESP32 utiliza `FSCom`/LittleFS para parte da configuração. O marco M3 deve adaptar todos os fluxos operacionais de load/save ao SD, incluindo boot, administração local, resets e migrações. Copiar canais para SD enquanto o núcleo os carrega de LittleFS deixaria duas fontes de verdade e não atende ao requisito.

M3 implementa a revisão de formato descrita em [M3_STATUS.md](M3_STATUS.md), com snapshots A/B binários, log de registros fixos e índice de segmentos reconstruído. O layout abaixo conserva a proposta de planejamento; o formato efetivo e seus limites estão no documento do M3. Definir serialização explícita e versionada; não salvar dumps de structs C++ com padding/ponteiros.

## Namespace exclusivo no cartão

```text
/CardputerADVMeshtastic/
  schema.json
  state/
    g000001/
      manifest.json
      device_state.pb
      config.pb
      module_config.pb
      channels.pb
      ui.json
    g000002/                 próxima geração completa
    active.json              seleção da geração + checksum
    previous.json            seleção anterior recuperável
  messages/
    segment-000001.log
    segment-000002.log
  indexes/
    conversations.idx        derivável dos logs
  drafts/
    drafts.json
  nodes/
    nodes.pb                 snapshot compatível com a base
  imports/
    canal.url
  exports/
    conjunto.url
  backups/
  diagnostics/
```

Não compartilhar diretório com dados do launcher ou do Tab5. Nome exato, extensões e layout upstream interno serão fechados ao implementar o adaptador.

## Dados a conservar

| Grupo | Campos mínimos |
|---|---|
| Preferências | Versão, offset UTC, notificações, aba/conversa selecionada, preferências pequenas da UI |
| Configuração do nó | Identidade/owner, material criptográfico necessário à identidade, região/preset/parâmetros LoRa, canais e módulos mantidos |
| Mensagem | ID local, sessão/boot, direção, origem, destino, broadcast/DM, identidade da conversa, slot observado, packet ID, texto UTF-8 integral, estado de envio, horários e validade |
| Metadados RX | RSSI/SNR/hops quando disponíveis, origem do horário e se é timestamp declarado pelo remetente |
| Rascunho | Identidade da conversa/destinatário, texto, cursor; URL em edição separada |
| Nós | ID, nomes, último RX, fonte/validade do tempo, posição/métricas e atualização de cada uma |

Limite de texto será obtido do protocolo selecionado e validado por **bytes UTF-8**, incluindo requisitos da modalidade de envio. Não importar o teto de 107 bytes do storage NVS do Tab5.

## Identidade de conversa

O slot 1 pode ser reutilizado para um canal diferente após importação. Mensagens antigas e rascunhos não podem migrar implicitamente para o novo canal.

Registrar ID local estável do canal e associá-lo à geração/configuração normalizada da chave efetiva e do nome. Mudança de nome/chave ou substituição deve ter política explícita de manter ou criar conversa; por padrão criar nova identidade quando a rede lógica mudar. Um hash técnico da chave não deve ser exibido em diagnósticos públicos. DM utiliza peer ID e modalidade/contexto exigidos pelo protocolo, mantendo o slot separadamente quando houver.

Deduplicação de recebidas por tupla de origem, packet ID e contexto da conversa, com janela temporal/boot apropriada para eventual reutilização de IDs. IDs de mensagens locais são independentes dos packet IDs RF.

## Horário correto entre reinícios

- Gravar UTC quando houver fonte válida; salvar também origem/qualidade.
- Antes da primeira hora GPS válida, usar monotônico + ID de boot. Não transformar isso em data falsa.
- Após sincronização, um ponto de ancoragem pode atribuir UTC estimado a eventos **do mesmo boot**, identificando a estimativa.
- Entre boots, monotônicos não se comparam. Sem UTC, exibir sessão/ordem e `hora desconhecida`.
- Ordenação durável usa sequência local, para não saltar mensagens após correção de hora GPS.
- Offset default proposto: UTC−4, configurável e salvo no SD; GPS fornece UTC, não fuso local.

## Logs e índices

Log append-only segmentado com cabeçalho versionado e registros de tamanho explícito + CRC cobrindo metadados/texto. Estado de TX muda por eventos que referenciam a mensagem; não reescrever todo o histórico por ACK. Último registro incompleto é ignorado/quarentenado; corrupção intermediária exige estratégia de ressincronização e indicação de erro.

Índice é derivável; pode ser reconstruído sem descartar mensagens. Página inicial de 20 registros em RAM, seguida de leitura sob demanda. Evitar scan completo bloqueante no boot: manifest/índice validado e reconstrução incremental com progresso.

Limites iniciais propostos: segmentos de 256 KiB e retenção total de 32 MiB ou 50.000 mensagens, o que ocorrer antes. Ajustar após medição; reservar espaço para geração nova e recuperação. Rotacionar somente segmentos antigos confirmados, informando política de retenção. Logs de diagnóstico têm limite separado e nunca carregam PSKs.

## Confirmação e corte de energia

Snapshots completos são gravados numa geração nova. Validar todos os arquivos e seu manifesto, flush/sync/close, e só então atualizar seleção ativa preservando anterior. No boot validar ponteiros e gerações; se o ponteiro ativo estiver incompleto, selecionar a última geração íntegra conforme protocolo de commit, sem misturar arquivos entre gerações.

FAT e controladores SD não garantem transação de vários arquivos nem atomicidade absoluta de rename em corte de energia. Definir marcadores/checksums, redundância e teste de interrupção; não anunciar garantia apenas por usar `.tmp` + rename.

Configuração de canal/identidade deve ser persistida e relida antes de confirmar sucesso à UI. Aplicação no núcleo pode exigir reinit/reboot; se falhar após commit, iniciar recuperação para geração anterior ou mostrar estado pendente claramente, sem sucesso total falso.

Histórico RX/TX vai para fila do worker e é sincronizado em prazo curto; política inicial até 1 s para mensagens e debounce de 1 s para rascunhos. Exibir erro se a fila saturar e distinguir `pendente` de `persistido`. Mudança de conversa/retorno/saída solicita flush do rascunho. Perda de energia antes do sync pode perder entradas pendentes; reduzir/medir janela em M3, sem prometer durabilidade inexistente.

## Falhas do cartão

| Condição | Comportamento previsto |
|---|---|
| Sem cartão no boot | Tela de recuperação/diagnóstico; não criar identidade nova nem entrar na rede com configuração descartável silenciosa |
| Cartão removido durante uso | Manter estado corrente em RAM por sessão; sinalizar `SD ausente / não salvo`; suspender mudanças de identidade/canais |
| SD cheio / somente leitura | Leitura disponível, erro de persistência visível; sem confirmação de configuração nova |
| Corrupção de última escrita | Recuperar geração/registro anterior íntegro; indicar recuperação |
| Schema mais novo desconhecido | Preservar arquivos; modo de recuperação, sem sobrescrever defaults |
| Cartão reinserido | Remontagem explícita/controlada; comparar identidade/geração antes de retomar escrita |

Durante falha após boot, RX pode continuar e o usuário pode ler a fila temporária. Envio com persistência indisponível precisa indicar claramente que o registro não será conservado, seguindo política fechada em M3. Não usar NVS como fallback oculto.

## Migração e segredos

Migração lê formato antigo, grava geração nova validada e preserva original até confirmação. Nunca usar a mudança de versão/build para apagar o diretório do produto. Auditar políticas upstream como autoformat e limpeza de prefs por versão antes de integrá-las.

PSKs e chaves de identidade são dados necessários no SD; não imprimir em logs/diagnósticos. Exportação `.url` contém a configuração compartilhável/chaves do canal e deve ser uma ação explícita. Backup completo contém segredos e permanece local. Criptografia do armazenamento em repouso é decisão posterior; este plano não a declara implementada.

## Extensão de navegação M4

M4 mantém schema 1, registros de 384 bytes e CRC do M3. Usa bytes reservados 304–331 para marcador de shell, página/seleção de nós, página/seleção/primeira linha do histórico, seleção de canal/ajuste e sequência/rolagem do detalhe. Não armazena o estado transitório de formulários ou diálogos: após reboot, abre a raiz da aba salva. Durante a sessão, troca de aba conserva esses estados em RAM. Rascunho e cursor continuam nos campos existentes, sem apagar logs ou snapshots.

Registros M3 sem marcador conservam o rascunho e traduzem vistas antigas para as novas abas. O build M3 ignora os campos novos e os zera ao salvar uma UI antiga; os pacotes M1–M3 já entregues permanecem preservados.

## Conversas e rascunhos M5

Schema 1/registro de 384 bytes continua. Bytes reservados 332–341 armazenam marcador de conversas, slot ativo e sequência de leitura de 64 bits. Identidade da conversa permanece derivada do nome/PSK efetiva, com slot apenas para seleção da UI. Não se expõe chave ou derivado em diagnóstico.

Além dos slots globais `drafts/a.bin` e `b.bin`, cada conversa usa `drafts/c<identidade>-a.bin` e `-b.bin`. A escrita escolhe a geração inativa daquela conversa independentemente da paridade da geração global, sincroniza/relê CRC e então publica o estado global. Troca é assíncrona pelo worker: conserva o contexto atual antes de carregar o próximo. Formulários transitórios permanecem em RAM. Cartão inteiro e dados M3/M4 não são formatados ou apagados.

Boot recupera a geração íntegra mais recente do contexto selecionado, inclusive quando o estado global ficou anterior à cópia já confirmada do contexto. Arquivo desconhecido ou perda de ambas as gerações conhecidas bloqueia escrita; uma geração íntegra permite recuperação. Marcador de leitura é monotônico e não retrocede quando a UI envia uma cópia anterior da navegação.

Histórico é filtrado por identidade antes de paginar e mantém a correlação dos estados de TX de M3. Índice de até oito canais ativos reconstrói não lidas em lotes de oito registros por ciclo do worker, com leitura das sequências salvas. `[N+]` na lista indica reconstrução em andamento; valores se estabilizam ao concluir. RX privado continua registrado, mas sua conversa não é apresentada nesta etapa (DM será M7). Ler a página mais recente no Chat marca a conversa como lida; compor, consultar outra aba ou ler página antiga conserva o indicador. Novos registros durante reconstrução são contados uma vez. Rotação de retenção solicita reconstrução do índice.

Worker M5 reserva stack de 8 KiB; log/fila/locks e limites de retenção permanecem de M3. Corte físico de energia e duração prolongada ainda exigem teste no ADV.

## URL digitada M6

Pares `drafts/url-a.bin`/`url-b.bin`: 2.080 bytes, magia URL6/versão 1, geração de 64 bits, comprimento/cursor e texto de até 2.047 bytes, CRC32. Worker salva após pausa de 1 s, valida readback; I/O em blocos de 256 bytes com o SPI liberado entre eles. Boot escolhe a maior geração íntegra; duas gerações conhecidas inválidas ou versão desconhecida bloqueiam escrita, preservando arquivos. Corte antes do flush pode perder a digitação pendente.

Exportações confirmadas são arquivos `.url` com readback em `exports/channels-<geracao>.url`; incluem chaves compartilháveis. Não há limpeza automática dessas exportações. Configurações de canal usam os snapshots completos existentes; revisão da proposta e validação do rádio antecedem o commit. Mudanças de identidade de conversa conservam contextos anteriores.
