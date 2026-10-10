# Análise do Tab5Meshtastic

Análise do código local em `../Tab5Meshtastic`, em 09/10/2026. HEAD: `fcdfe23a024b62949955d70c688b09286663656a`; `git status --short` sem alterações no momento da leitura. O projeto ADV não altera essa base.

## Constatação central

O Tab5 executa uma interface para um nó Meshtastic externo. `main/main.cpp` escolhe `ble_transport` ou `UartTransport` + `MeshSession`; mensagens e administração seguem para esse nó usando protobufs da API cliente.

O Cap oficial do ADV apresenta um SX1262 por SPI e um GPS por UART. Ele não oferece um processador executando Meshtastic e uma API `ToRadio/FromRadio`. A conclusão arquitetural, a partir do hardware e do suporte oficial, é executar **núcleo Meshtastic + interface no ESP32-S3**. Copiar `UartTransport` do Tab5 não resolve a comunicação LoRa. Ver [fontes](REFERENCES.md).

## Inventário baseado no código

| Função | Evidência local | Decisão para o ADV |
|---|---|---|
| Chat por canal, seleção e rascunhos em sessão | `main/ui/ui_shell.cpp`: `append_messages`, `chat_channel_cb`, `do_send` | Preservar comportamento, refazer renderização e persistir rascunhos no SD |
| Envio por fila e feedback do rádio | `main/protocol/mesh_session.cpp` e `.h`: `OutgoingText`, feedback, retry, `send_text` | Aproveitar semântica; adaptar aos eventos do núcleo, sem copiar temporização UART |
| Mensagens diretas | `app_commands.h`, `main.cpp`, `MeshSession::send_dm`, `ui_shell.cpp`: `dm_peer`, filtro e botão no detalhe do nó | Existem no código; prever DM local e persistência completa |
| Nós com nome, sinal, hops, posição e métricas | `main/app/app_state.h`, `app_state.cpp`, `mesh/mesh_proto.cpp`, tela de detalhes | Lista compacta e detalhe paginado; reutilizar regras de validade |
| URL Meshtastic / ChannelSet | `main/protocol/channel_url.cpp` e `.h` | Melhor candidato a portar: lógica independente de UI/rádio, após alinhar protobufs |
| Validação UTF-8, PSK e Base64 URL-safe | `channel_url.cpp`: `channel_settings_valid`, `channel_url_decode` | Preservar validação por bytes e limites; agregar testes de entrada digitada |
| Chave herdada ao exportar/adicionar secundário | `channel_settings_resolve_inherited_key` | Preservar resolução contra o primário da origem |
| Oito slots, criação, edição e importação | `main/protocol/channel_service.cpp`, `ui/screens/channels_screen.cpp` | Reutilizar regras/preflight; substituir administração remota por configuração local |
| Confirmação após escrever canal | `channel_service.cpp`: `write_channel`, `same_channel`, worker | No ADV: validar estado do núcleo e persistência no SD, incluindo reinício |
| Importação com falha parcial | Worker informa resultados reais; grava canais sequencialmente, sem transação geral | ADV deverá persistir/aplicar uma geração completa; não afirmar atomicidade por herança |
| Câmera e QR | `board/channel_camera.*`, `channels_screen.cpp` | Remover câmera. Exportar URL/arquivo; QR pequeno fica fora do MVP |
| Preferências | `storage/settings_store.cpp`: namespace NVS `t5app` | Reescrever backend para SD |
| Histórico | `storage/message_store.cpp`: 30 slots NVS, texto de até 107 bytes | Reescrever: log paginado no SD, sem truncar silenciosamente |
| Clock | `app/app_clock.cpp`: RTC do Tab5, NVS, offset; `network/wifi_service.*`: SNTP | Usar hora GPS + monotônico e offset salvo no SD |
| Localização própria | `network/position_service.cpp`: consulta HTTP por IP | Substituir por GPS físico, sem dependência de rede |
| Bateria e som | `board/battery_monitor.*`, `tab5_audio.*` | Regras de apresentação úteis; drivers próprios do ADV, alerta simples |
| Display e teclado | LVGL em `ui_shell.cpp`; teclado A164; `board/display_idle.*` | Refazer para ST7789/TCA8418; menu oculto é independente de apagar tela |
| Traceroute | Não foi identificado fluxo de solicitação/exibição no Tab5 analisado | Recurso novo e posterior, não um simples porte |
| Build OTA | `build_ota.sh`: build/cópia/checksum, mas aceita gravação opcional por porta | Aproveitar ideia de artefato/checksum; criar ferramenta futura sem upload |

## Limitações que não devem ser transportadas

`message_store_append` ignora `to_node`, `packet_id`, horário absoluto, RSSI/SNR e tipo broadcast. O registro NVS guarda direção, canal, remetente, tempo monotônico e texto. `message_store_load_page` ignora o filtro de canal. Isso impede tratar o histórico do Tab5 como um banco completo de conversas diretas ou de confirmações.

`AppState` mantém até 256 nós e 64 mensagens; os registros de mensagem não incluem estado persistente de entrega nem identidade de canal durável. No ADV, não copiar arrays/snapshots de toda a base para renderizar uma tela. Manter uma página de registros e aproveitar o banco de nós do núcleo.

O sucesso de fila local não comprova entrega ao destino. O novo produto precisa separar rascunho, fila, transmissão, ACK quando houver e falha/resultado desconhecido. Retries devem seguir o núcleo, sem criar um segundo mecanismo de retransmissão.

`channel_service` usa `AdminMessage`, passkey de sessão, pedidos/respostas e readback por rádio externo. Esses detalhes não servem de backend local para o ADV. Aproveitar invariantes e revisão de importação, implementando o adaptador de configuração do núcleo.

## Documentos antigos versus código atual

- `docs/ARCHITECTURE_CURRENT.md` ainda descreve fase inicial BLE-only, ausência de `admin.pb` e ausência de persistência; o código já tem UART, administração e histórico NVS.
- `docs/STORAGE_SCHEMA.md` descreve arquivos SPIFFS, retenção extensa e paginação por canal; o caminho ativo de mensagens/preferências usa NVS. `app_storage.cpp` monta SPIFFS, porém não implementa esses repositórios de arquivos.
- O README afirma que DMs não foram implementadas; há envio, recepção/registro em RAM, botão e filtro de DM no código atual. Isso não comprova sua persistência completa ou validação no hardware.
- `UPSTREAM.md` também tem descrições históricas de armazenamento. Referência documental é útil, mas a decisão de porte deve seguir os fontes e os testes.

## Reaproveitamento recomendado

1. Portar o codec de URL e suas validações, alinhando tipos nanopb com o upstream escolhido.
2. Transportar as regras de canais e o desenho de estado/comandos; escrever serviços locais novos.
3. Aproveitar casos de `tests/channels_test.cpp`, `channel_service_test.cpp`, `chat_clock_test.cpp`, `chat_ui_test.cpp` e `mesh_proto_test.cpp` quando os marcos correspondentes forem implementados.
4. Reescrever UI, armazenamento e integrações de hardware; retirar o sistema de transportes e recursos de rede.

No planejamento inicial nenhum fonte foi copiado. Em M0/M1 foi obtido checkout Meshtastic fixado e integrado por overlay de build; o Tab5 continua sendo referência de análise, sem fontes copiados nesses marcos. Ver [status atual](M0_M1_STATUS.md).
