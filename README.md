# Cardputer ADV Meshtastic

Cliente Meshtastic standalone para **M5Stack Cardputer ADV + Cap LoRa-1262 oficial (SX1262, GPS e microSD)**. O núcleo Meshtastic executa no próprio ADV; o uso normal dispensa celular, computador e internet.

**Versão 1.0.1** — [downloads e notas da release](https://github.com/tpklge/CardputerADVMeshtastic/releases/tag/v1.0.1).

## Recursos

- Chat em uma única lista contínua por canal ou DM: mensagens recentes embaixo, enviadas e recebidas com cores e identificação distintas. Cima/baixo percorrem o histórico uma mensagem por vez, sem troca de páginas.
- Enter abre a composição; Fn+Enter envia. Cima/baixo continuam navegando pelo histórico enquanto você escreve. Receber mensagens preserva o rascunho e o cursor. Direita na lista abre o texto completo e os dados da mensagem.
- Canais editáveis, criação/importação de links digitados e exportação para microSD.
- Conversas privadas com PKI, consulta de nós, busca, favoritos, posição, distância e rumo quando os dados são válidos.
- Traceroute manual, diagnóstico, backup de configuração e recuperação pelo próprio ADV.
- Tela apaga após 30 segundos de inatividade; teclado ou mensagens recebidas reacendem a tela. Mensagens de qualquer canal ou DM emitem um bipe forte, mantendo rádio e SD ativos.
- Quatro temas de alto contraste e fonte adaptada à tela de 240 × 135.
- Histórico, rascunhos, preferências, identidade e configuração no microSD. Hora do GPS, com fuso ajustável.

Tab abre o menu inferior e a barra de estado; quando o menu está visível, Tab muda de aba e Shift+Tab volta. Ambos somem após cinco segundos da abertura ou troca de aba. Setas e Esc funcionam sem Fn; Fn nas teclas compartilhadas permite digitar pontuação. A bateria aparece como porcentagem estimada no canto superior direito quando a barra está visível.

## Instalação por OTA

Baixe **`cardputer-adv-1.0.1-ota.bin`** na [release v1.0.1](https://github.com/tpklge/CardputerADVMeshtastic/releases/tag/v1.0.1) ou em [newversion/1.0.1](newversion/1.0.1). Use o fluxo de instalação OTA do **M5Launcher já instalado**. O pacote contém somente a aplicação: não inclui bootloader ou tabela de partições.

A versão 1.0.1 foi confirmada funcionando no ADV pelo usuário. O pacote publicado preserva exatamente o binário testado. A versão/layout/tamanho do slot do launcher instalado ainda não foram medidos; o manifesto conserva esses campos como não verificados. Este projeto não altera as partições do aparelho.

O [binário 0.8.2 previamente testado](newversion/m8-0.8.2) também fica preservado no repositório. Guias M0–M8 registram as etapas anteriores; consulte [o uso atual](docs/M8_USAGE.md) e [as notas de 1.0.1](newversion/1.0.1/RELEASE_NOTES.md).

## Compilar

Requer Python 3.11, Git e a cadeia fixada em [requirements-build.txt](requirements-build.txt) e [platformio.ini](platformio.ini): PlatformIO 6.2.0, espressif32 6.13.0 e LVGL 8.3.11. Meshtastic e protobufs são fixados em [upstream.lock.json](upstream.lock.json).

```sh
git clone --recurse-submodules https://github.com/tpklge/CardputerADVMeshtastic.git
cd CardputerADVMeshtastic
git checkout v1.0.1
git submodule update --init --recursive
python3.11 -m venv .venv
.venv/bin/python -m pip install -r requirements-build.txt
.venv/bin/python tools/prepare_upstream.py
.venv/bin/python tools/build_ota.py --release
```

O resultado fica em `newversion/1.0.1`. O build usa `default_8MB.csv` apenas como referência de linkedição. Os scripts de entrega não acessam porta serial nem enviam firmware ao dispositivo.

```sh
.venv/bin/python tools/test_native.py
.venv/bin/python tools/test_lvgl.py
```

No macOS, os dois testes aceitam `--sdk /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk`. O teste LVGL requer as dependências resolvidas de `cardputer-adv-m7`; execute `.venv/bin/pio pkg install -e cardputer-adv-m7` antes, caso esse ambiente ainda não tenha sido preparado. Testes nativos usam ASan/UBSan e fakes de hardware; não substituem ensaios físicos de rádio, FAT e concorrência SPI/FreeRTOS.

## Dados e limites

O histórico contínuo consulta os registros do SD conforme a rolagem, mantendo uma janela pequena em RAM. Retém até **49.152 registros**: mensagens, estados de entrega, posições, telemetria e sessões. Quando esse limite é alcançado, substitui o segmento mais antigo. Backups de configuração preservam os arquivos de histórico e rascunhos.

Não há Wi-Fi, BLE, câmera, ajuste de brilho ou outro rádio suportado. Envios exigem configuração válida e histórico durável no microSD. Gravação antes do TX, restauração e backups podem ocasionar espera de SD; medições prolongadas de latência e concorrência no aparelho continuam sendo melhorias futuras.

## Licença e fontes

**GNU GPL versão 3**, conforme o [LICENSE existente no repositório](LICENSE), preservado nesta publicação. As fontes completas da aplicação são publicadas em `main` e no tag `v1.0.1`; o submódulo Meshtastic fixa a revisão utilizada e mantém os avisos/licenças upstream. As dependências conservam suas próprias licenças. Veja [UPSTREAM.md](UPSTREAM.md) e [os componentes de terceiros](THIRD_PARTY.md).
