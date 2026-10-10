# Plano de validação futura

Esta matriz orienta os marcos de implementação. M0/M1 já têm build e testes nativos de teclado/empacotamento aprovados, registrados em [status](M0_M1_STATUS.md); os casos físicos continuam pendentes. Hardware comprova rádio, SPI, GPS e instalação.

| Área | Caso | Resultado esperado | Marco / meio |
|---|---|---|---|
| Base | Dependências/revisões fixadas | Build reprodutível e alvo ESP32-S3 | M0, revisão/build offline |
| OTA | Imagem do app e tamanho | Formato correto e dentro do limite do instalador | M0/M8, inspeção offline |
| Boot | Inicialização fria com SD/Cap | Identidade/configuração preservadas; UI utilizável | M1–M3, hardware |
| SPI | RX/TX durante escrita/rotação SD | Sem colisão, deadlock ou corrupção; medir latências e drops | M1/M3, hardware |
| Menu | 4.999 ms / 5.000 ms após abrir | Visível / oculto | M4, lógica nativa |
| Menu | Troca real / mesma aba | Renovar / não renovar deadline | M4, lógica nativa |
| Menu | Digitação, rolagem, RX e GPS | Não renovar deadline; foco/rascunho preservados | M4, lógica + UI |
| Menu | Correção da hora GPS | Não alterar timeout monotônico | M4/M5, lógica |
| Teclado | Símbolos de URL, modifiers e repetição | Link digitável integralmente; envio explícito | M1/M4/M6, hardware |
| UTF-8 | Acentos e limites por bytes | Sem quebra/truncamento silencioso | M5/M6, nativo + UI |
| Chat | Alternar canais e reiniciar | Destino, histórico e rascunhos corretos | M5, integração |
| Chat | RX enquanto lê página antiga | Indicador de nova mensagem, sem roubar foco/página | M5, UI |
| Envio | Fila local, TX, erro, ACK/timeout | Estados distinguíveis; sem entregar por simples aceitação | M5/M7, integração |
| Nós | Sem sinal/posição/hops/telemetria | `--`/desconhecido, sem inventar valores | M5, nativo + UI |
| GPS | UTC válido sem fix / fix sem UTC confiável | Validar tempo e posição separadamente | M5, nativo + hardware |
| Clock | Boot sem hora, holdover, offset UTC−4 | `--:--` quando desconhecido; qualidade conservada | M5, nativo + hardware |
| SD | Histórico amplo e paginação | Memória limitada, sem carregar todo o arquivo | M3/M5, integração |
| SD | Remoção, cheio, somente leitura | Erro visível; configuração não confirmada como salva | M3, hardware |
| SD | Corte durante append/commit | Recuperar registros/geração íntegros e apontar pendências | M3, falhas simuladas + hardware |
| SD | Índice corrompido | Reconstrução incremental sem apagar histórico | M3, integração |
| SD | Troca de cartão/identidade | Bloquear mistura de dados e escrita equivocada | M3, integração |
| SD | Upgrade/downgrade de schema | Migração não destrutiva ou recuperação | M3/M8, integração |
| Canais | Nome/PSK inválidos; limites | Rejeitar antes de aplicar | M6, nativo |
| Links | Base64/padding/protobuf inválidos | Erro sem alteração no núcleo ou SD | M6, nativo |
| Importação | Poucos slots, duplicatas e chave herdada | Preflight correto, origem preservada | M6, nativo + integração |
| Importação | Substituição e parâmetros LoRa | Um primário; preservar região/potência local | M6, integração |
| Conversa | Reutilização de slot após importação | Histórico/rascunho não enviados ao canal novo | M6, nativo + UI |
| Canais | Criar/importar, reiniciar, comunicar | Estado durável e interoperabilidade real | M6, hardware |
| DM | Destinatário e recepção após reinício | Conversa privada correta e dados completos | M7, integração + hardware |
| Traceroute | Sucesso/falha/timeout | Consulta limitada e resultado paginado | M7, hardware |
| Release | Executar versão instalada por OTA | Boot e dados preservados pelo instalador existente | M8, hardware via OTA |

## Medições de release

- Tamanho do app versus limite real do instalador, com margem documentada.
- Heap mínimo/largest block e stacks sob maior tráfego esperado, editor de URL e paginação SD.
- Maior duração de posse do SPI e responsividade do teclado sob gravação.
- Perdas de RX, erros SD e fila cheia observados em sessão prolongada.
- Tempo de boot com índice válido e com reconstrução necessária.
- Janela de mensagens/rascunhos ainda pendentes ao cortar energia.

Valores de aprovação de latência/memória serão fixados após M1/M2 fornecerem uma linha de base; não inventar capacidade de hardware no planejamento. Aprovar MVP somente após casos obrigatórios de M0–M6, interoperabilidade e preservação em reinício.
