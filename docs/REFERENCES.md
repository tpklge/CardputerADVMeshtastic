# Fontes e evidências

Consulta realizada em 09/10/2026. Links de branch móvel servem para documentação; links de código com SHA identificam a revisão pesquisada.

## Código local

- `../Tab5Meshtastic`, HEAD `fcdfe23a024b62949955d70c688b09286663656a`, sem alterações locais no momento da análise.
- Fontes centrais: `main/main.cpp`, `main/app/app_state.h`, `main/app/app_commands.h`, `main/protocol/channel_url.cpp`, `main/protocol/channel_service.cpp`, `main/protocol/mesh_session.cpp`, `main/mesh/mesh_proto.cpp`, `main/ui/ui_shell.cpp`.
- Persistência/hora: `main/storage/message_store.cpp`, `settings_store.cpp`, `app_storage.cpp`, `main/app/app_clock.cpp`, `main/network/position_service.cpp`.
- Documentos confrontados: README, `docs/ARCHITECTURE_CURRENT.md`, `docs/STORAGE_SCHEMA.md`, `docs/LAUNCHER_INTEROP.md`, `UPSTREAM.md`; script `build_ota.sh`.
- O projeto `../Meshclient-CardputerC6L/meshclient` foi consultado apenas como contexto local. Sua arquitetura de rádio externo não é a base para o Cap oficial.

## Hardware e upstream primários

- [M5Stack Cardputer ADV](https://docs.m5stack.com/en/core/Cardputer-Adv): especificação do controlador, tela, microSD e periféricos.
- [M5Stack Cardputer Mesh Kit](https://docs.m5stack.com/en/core/Cardputer_Mesh_Kit): composição ADV + Cap, interfaces, pinagem e inicialização do switch RF.
- [Variante Cardputer ADV — Meshtastic](https://github.com/meshtastic/firmware/blob/e5998edcb35db8cbda421ae9a23c332970f79826/variants/esp32s3/m5stack_cardputer_adv/variant.h): sinais SX1262/GPS/display e configuração de hardware.
- [Ambiente PlatformIO da variante](https://github.com/meshtastic/firmware/blob/e5998edcb35db8cbda421ae9a23c332970f79826/variants/esp32s3/m5stack_cardputer_adv/platformio.ini): ambiente existente `m5stack-cardputer-adv`.
- [Inicialização extra do ADV](https://github.com/meshtastic/firmware/blob/e5998edcb35db8cbda421ae9a23c332970f79826/src/platform/extra_variants/m5stack_cardputer_adv/variant.cpp): Cap/expansor RF e áudio.
- [MeshService](https://github.com/meshtastic/firmware/blob/e5998edcb35db8cbda421ae9a23c332970f79826/src/mesh/MeshService.h): caminhos de integração de pacotes/eventos, sujeitos ao runtime e ownership upstream.
- [Channels](https://github.com/meshtastic/firmware/blob/e5998edcb35db8cbda421ae9a23c332970f79826/src/mesh/Channels.h): configuração local de canais e notificação de mudanças.
- [NodeDB](https://github.com/meshtastic/firmware/blob/e5998edcb35db8cbda421ae9a23c332970f79826/src/mesh/NodeDB.cpp) e [FSCommon](https://github.com/meshtastic/firmware/blob/e5998edcb35db8cbda421ae9a23c332970f79826/src/FSCommon.h): persistência upstream que deve ser adaptada ao cartão.
- [Flags de configuração](https://github.com/meshtastic/firmware/blob/e5998edcb35db8cbda421ae9a23c332970f79826/src/configuration.h): exclusões de recursos, a verificar na base escolhida.

A API GitHub informou `e5998edcb35db8cbda421ae9a23c332970f79826` como HEAD de develop na consulta, com timestamp de commit `2026-10-09T03:34:20Z`. Isso apenas fixa a pesquisa; seleção de release para implementação permanece no marco M0.

## O que é decisão do projeto

Arquitetura de UI, quatro abas, layouts, timeout do menu, limites de cache/log e protocolos de recuperação propostos são decisões de planejamento. Não são funções já entregues pelo firmware oficial nem características comprovadas por esses links.

O núcleo embarcado é uma conclusão técnica baseada no Cap SPI/GPS e no suporte da variante. Durante M0/M1, o usuário confirmou M5Launcher; versão/partições instaladas continuam desconhecidas.

## Fontes da implementação M0/M1

- [Release Beta selecionada](https://github.com/meshtastic/firmware/releases/tag/v2.7.26.54e0d8d) e [código fixado](https://github.com/meshtastic/firmware/tree/54e0d8d0ab2ff56b3a9ce967e53f79e49af560fb): checkout reproduzível em `vendor/meshtastic-firmware`.
- [M5Launcher 2.9.1](https://github.com/bmorcelli/Launcher/tree/2.9.1), [variante Cardputer](https://github.com/bmorcelli/Launcher/blob/2.9.1/boards/m5stack-cardputer/platformio.ini) e [tabela inicial](https://github.com/bmorcelli/Launcher/blob/2.9.1/support_files/custom_8Mb.csv): pesquisa do instalador, sem presumir que essa versão/layout esteja no ADV.
- Arduino-ESP32 2.0.17: pacote local fixado. `cores/esp32/esp32-hal-misc.c` mostra autoerase após falha NVS; `libraries/SD/src/SD.cpp` e `libraries/SPI/src/SPI.cpp` permitem conferir montagem e inicialização do SPI já ativo.
- Bibliotecas/arquivos de licença foram conferidos nos pacotes locais `.pio/libdeps/cardputer-adv-m1`; versões/revisões em `upstream.lock.json` e `UPSTREAM.md`.
