# M3 — persistência SD, candidato 0.3.0

Implementação autorizada em 09/10/2026. M2 0.2.1 tem confirmação do usuário de persistência, troca de região e RX/TX de mensagens. O candidato M3 passou em build/testes offline; O usuário confirmou funcionamento e persistência, incluindo rascunho após reboot. Testes físicos de interrupção de escrita e duração prolongada continuam pendentes. O agente não enviou firmware.

## Entrega

- Migração conservadora da configuração M2: identidade, chaves, canais, LoRa e cache de nós permanecem no microSD. Antes de iniciar o núcleo sobre dados existentes, cria um snapshot validado.
- Dois slots de snapshots completos (`state/a`, `state/b`), incluindo device/config/module/channels/nodes. Manifesto binário versionado, geração de 64 bits, tamanho e CRC32 de cada arquivo, mais CRC32 do manifesto. Grava o slot inativo e publica o manifesto por último; valida arquivos antes de confirmar sucesso. Boot escolhe a maior geração íntegra e restaura o conjunto sem misturar gerações.
- Marcador `state/m3.started` impede fallback a arquivos avulsos quando não há snapshot recuperável após a migração. Versões desconhecidas são preservadas/bloqueadas. Temporários e arquivos substituídos na recuperação usam nomes únicos `.saved-N`; nenhum formato automático.
- Mutex de configuração recursivo engloba saveToDisk; gravação isolada de nós também gera snapshot. Chamadas internas durante uma gravação completa não publicam snapshots intermediários. Saves não reformatam nem entram em retry quando já há falha de storage.
- No M3, SafeFile verifica CRC32 e comprimento, substituindo a verificação XOR como critério de aceitação. Readback usa blocos de 256 bytes sem manter o SPI preso durante a leitura inteira. Cópias/digests de snapshots também usam blocos de 256 bytes. Carga de protobuf no boot ainda mantém o lock upstream; ocorre antes da inicialização do rádio.
- Worker de histórico com fila de 16 registros copiados, stack de 6 KiB e prioridade 1. Logs com cabeçalhos versionados/CRC e registros fixos de 384 bytes, com serialização explícita little-endian; não há dumps de structs.
- Persistência integral do texto suportado no M2 (até 200 bytes UTF-8), direção, origem/destino, packet ID/token, conversa, sessão, monotônico, UTC/validade, horário RX declarado, RSSI/SNR, hops observados, posição e métricas disponíveis. IDs RF não substituem a sequência local durável.
- Estados de fila, TX_DONE, feedback e erro são eventos append-only. Consulta de página correlaciona os estados mais recentes por packet ID/token e sessão; nunca declara entrega por TX_DONE. Configuração bem-sucedida é conservada nos snapshots, sem registrar chaves no log de eventos.
- Antes de TX, a fila local com texto é gravada, sincronizada e relida pelo worker. Se a persistência ou o limite de espera de 1 s falhar, o pacote é liberado e TX é cancelado. RX e feedback são assíncronos; queda antes do flush pode perder entradas pendentes. Falha/saturação aparece na UI.
- Conversa usa identidade derivada de nome e PSK efetiva (expansão/herança oficial), mais peer quando privado, sem usar o slot como identidade. Não expõe o derivado da chave em diagnósticos. DM pela UI e seu contexto PKI completo continuam no M7.
- Deduplicação persistente limitada a 32 fingerprints: origem/packet ID/conversa e janela de uma hora. Mesmo boot usa monotônico; entre boots só compara quando ambos têm UTC válido. Boot reidrata fingerprints dos 32 registros mais recentes; deduplicação RF oficial permanece no núcleo.
- Rascunho, cursor, aba, identidade de conversa e offset UTC usam dois arquivos com geração/CRC. Debounce de 1 s; envio/troca de aba solicita flush. Rascunho com contexto diferente é preservado em `drafts/context-...bin`, sem associação silenciosa a outro canal. Interface de seleção desses rascunhos depende do marco de canais.
- Cache upstream de nós é salvo até cerca de 1 s após o primeiro evento da janela, em batches; não se duplica uma base inteira de nós em RAM da aplicação. Cortes antes desse prazo podem perder atualizações pendentes.
- Índice de segmentos é reconstruído pelos cabeçalhos no boot (96 entradas). Validação de registros antigos é incremental; páginas de quatro mensagens são lidas sob demanda. Corrupção de um registro não desloca os seguintes; tail incompleto fica no segmento anterior e abre um novo. Não há scan completo bloqueante nem array de todo o histórico.
- Retenção de 96 segmentos × 512 registros, inferior a 19 MiB/50 mil registros incluindo eventos. Retira somente o slot mais antigo ao rotacionar, nunca o ativo. Conteúdo anterior ao M3 que não existia em log não é inventado/importado como histórico completo.
- Sem SD no boot, núcleo parado sem gerar identidade. Falha após boot conserva RX em RAM por sessão; TX e mudanças persistentes são bloqueados. Remontagem exige reinício. Sem NVS alternativo.
- Fonte nominal 12 (20% maior), raster 16; layout revisado e preview de 240×135 conferido. Tela de histórico provisória permite ler mensagens completas por trechos; não antecipa o shell/menu definitivo do M4.

## Formato em uso

```text
/CardputerADVMeshtastic/
  schema.bin                    ADP3, versão 1, CRC
  core/prefs/*.proto            área de trabalho oficial já usada pelo M2
  state/a/*.proto + manifest.bin
  state/b/*.proto + manifest.bin
  state/m3.started
  messages/s00.log ... s95.log  ADL3, versão 1, cabeçalho 32 bytes
  drafts/a.bin, b.bin           ADV3, versão 1, registros 384 bytes
  drafts/context-*.bin         rascunhos de contextos anteriores
```

CRC detecta corrupção; não é autenticação. Configuração/chaves permanecem legíveis no cartão conforme escopo. FAT/controlador podem perder mais que a última operação em interrupção física; estes mecanismos não são garantia de atomicidade absoluta.

## Validação local

- Build M3 aprovado: aplicação OTA **745.936 bytes**; RAM estática **84.368 bytes**, flash linkada **745.513 bytes**. O limite de link é referência offline; tamanho do slot instalado ainda desconhecido.
- Testes C++ com ASan/UBSan: teclado, fila/validação mesh, substituição FAT com falhas e codecs de storage/snapshot. CRC padrão e incremental, metadados, UTF-8, 200 bytes, invalidação por qualquer byte alterado, todos os cortes de registro/manifesto, rejeição de schema futuro, seleção do snapshot anterior e deduplicação temporal/contextual.
- Builds de regressão M1 e M2 aprovados; pacotes anteriores preservados.
- Seis testes Python de imagem/empacotamento, incluindo M3/schema/runtime e ausência de afirmação de validação física.
- Preview gerado diretamente dos bitmaps da fonte, sem imagem externa ou alteração de bibliotecas. Fontes do vendor permanecem fixadas e sem alterações.

## Validação no ADV

Seguir [M3_USAGE.md](M3_USAGE.md): boot com SD M2, identidade/canais/região conservados, rascunho após reinício, histórico RX/TX, cache de nós e fonte maior. Testes de SD cheio/removido/somente leitura, interrupção de escrita, latência SPI e duração prolongada permanecem físicos e pendentes. Não afirmar M3 estabilizado apenas por testes de formato.
