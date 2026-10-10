# Componentes de terceiros

A aplicação é distribuída sob GNU GPL versão 3, conforme `LICENSE`. Avisos e licenças dos componentes originais permanecem nos respectivos projetos/fontes; o checkout Meshtastic não é modificado.

- Meshtastic firmware: `v2.7.26.54e0d8d`, commit `54e0d8d0ab2ff56b3a9ce967e53f79e49af560fb`, submódulo `vendor/meshtastic-firmware`. Repositório: https://github.com/meshtastic/firmware.
- Meshtastic protobufs: commit `6b1ded439633cd03d4af85b44231b91d1d106278`, submódulo do firmware. Repositório: https://github.com/meshtastic/protobufs.
- LVGL: versão 8.3.11, configurada em `platformio.ini`. Sua licença acompanha as fontes da dependência.
- Demais bibliotecas e cadeia ESP32: declarações em `platformio.ini`, arquivos do firmware upstream e inventário de pacotes no manifesto da release. Cada dependência preserva a licença fornecida pelo autor.

Para obter as fontes, faça clone com `--recurse-submodules`, selecione `v1.0.1` e execute `git submodule update --init --recursive`. O script `tools/prepare_upstream.py` verifica os commits fixados antes da compilação; `tools/stage_core.py` aplica o overlay em `.pio`, sem editar o submódulo original. A release inclui também um arquivo de fontes com os submódulos e bibliotecas resolvidas utilizadas no build.
