# Base e atribuição

- Meshtastic firmware: https://github.com/meshtastic/firmware
- Tag: `v2.7.26.54e0d8d` (Beta); commit `54e0d8d0ab2ff56b3a9ce967e53f79e49af560fb`.
- Licença da base: GNU GPL versão 3; cópia em `LICENSE`.
- Submódulos e bibliotecas fixados em `upstream.lock.json`.

M1 compila os fontes upstream `src/SPILock.cpp`, `src/concurrency/Lock.cpp`, `src/concurrency/LockGuard.cpp` e `src/input/TCA8418KeyboardBase.cpp`, consultando `variants/esp32s3/m5stack_cardputer_adv/variant.h`. O checkout não recebe patches; `tools/pio_build.py` integra os fontes externos ao build do projeto.

O mapa físico do teclado se baseia na ordem da matriz de `src/input/CardputerKeyboard.cpp`. O decoder próprio rastreia pressionamento/soltura de cada tecla e não incorpora atalhos de reboot/GPS/Bluetooth da UI original. Mapeia `-` sem Shift e `_` com Shift, alinhado aos símbolos usuais do editor.

A inicialização do expansor RF segue os registradores da variante oficial. A HAL RadioLib própria usa o mesmo `spiLock` e a mesma disciplina de `spiBeginTransaction`/`spiEndTransaction` da base, adicionando medições.

Bibliotecas de M1:

| Componente | Uso | Licença declarada nos pacotes |
|---|---|---|
| RadioLib 7.6.0 | SX1262 e HAL Arduino | MIT |
| Meshtastic TinyGPSPlus | NMEA | LGPL-2.1 |
| Meshtastic OLEDDisplay | Buffer/texto | MIT |
| Meshtastic ST7789 | Display colorido por scanline | MIT |
| nanopb 0.4.9.1 | Headers dos protobufs upstream | zlib |
| Meshtastic ArduinoThread | Headers de concorrência upstream | MIT |
| Arduino-ESP32 2.0.17 | Framework e bibliotecas SPI/SD/Wire | LGPL-2.1 e componentes com licenças próprias |

Conservar os arquivos de licença/avisos de cada dependência no checkout/pacotes e em eventual distribuição de fontes. O código próprio integra uma obra GPLv3. Nenhum fonte do Tab5Meshtastic foi copiado em M0/M1.

## Integração M2

`tools/stage_core.py` materializa uma cópia de build e adapta startup/loop, FSCom, mutexes, salvaguardas de storage e callback de TX_DONE. A base rastreada não é alterada. Rádio e GPS usam drivers oficiais; não há segundo RadioLib SX1262/TinyGPS no build M2. A variante extra de áudio original é excluída, e os registradores do Cap são inicializados pelo produto. Detalhes em [M2_STATUS.md](docs/M2_STATUS.md).

Dependências adicionais fixadas em `m2_libraries` do lock: OneButton (BSD), arduino-fsm (MIT), ErriezCRC32 (MIT), NonBlockingRTTTL (MIT) e rweather/Crypto 0.4.0 (MIT, com avisos nos fontes). São dependências do núcleo; áudio/botões do produto continuam excluídos. Conservar atribuições também do código criptográfico e de compressão incorporado pelo upstream.

## Integração M3 e fonte

O overlay M3 mantém o runtime/driver fixado e acrescenta checkpoints de configuração, readback CRC32 por blocos e acesso somente leitura à PSK efetiva para identidade local da conversa. `include/cardputer_adv/font12.hpp` deriva da ArialMT_Plain_10 do OLEDDisplay fixado (MIT), reproduzível por `tools/generate_font12.py`; conservar os avisos da biblioteca. Nenhum fonte do Tab5 foi copiado nesta etapa.

## Integração M6

`channel_url.hpp`/`src/protocol/channel_url.cpp` foram adaptados de `Tab5Meshtastic/main/protocol/channel_url.{h,cpp}`; fixture `channels-eight.url` vem dos testes do Tab5. O planner e UI são próprios do ADV. Codec usa os protobufs/Nanopb da revisão fixada; não transporta UART, câmera/QR ou componentes ESP-IDF do Tab5. O checkout do Tab5 foi preservado.

## Interface LVGL 0.7.2

LVGL 8.3.11, pacote PlatformIO `lvgl/lvgl@8.3.11`, licença MIT, upstream https://github.com/lvgl/lvgl/tree/v8.3.11. Bibliotecas resolvidas registradas no manifesto OTA. A fonte Adv_Plain_12 mantém sua atribuição anterior; o adaptador converte as colunas OLED para bitmap LVGL de 1 bpp e preserva os caracteres Latin-1. Port TFT usa inicialização/rotação do ST7789 já fixado, com flush RGB565 próprio no HSPI exclusivo da tela.

## Consolidação M8 0.8.0

O overlay expõe getters somente leitura de estado, resultado e cooldown do TraceRouteModule para a UI sem tela upstream. startTraceRoute, protobuf, roteamento, ACK/retries e timeout continuam no núcleo fixado. Vendor não foi alterado. LVGL 8.3.11 e a fonte anterior continuam; temas são paletas RGB565 da aplicação. Configuração e backups mantêm protobuf versão 24, com revisão e ID local verificados antes da restauração.
