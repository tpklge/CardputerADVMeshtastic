# Ajustes antes de M8 — 0.7.2

Em 10/10/2026, o usuário confirmou 0.7.1 e pediu Enter antes de editar mensagens, setas/Esc sem Fn, LVGL se possível e cores distintas para envio/recepção. Esta revisão entrega esses ajustes e foi confirmada funcionando pelo usuário antes de autorizar M8. Este registro descreve a entrega 0.7.2; a consolidação posterior está em [M8_STATUS.md](M8_STATUS.md).

## Mudanças

- Chat exige Enter para abrir o editor em canal/DM. Digitação, Backspace ou Fn+Enter na lista preservam o rascunho e não enviam. Seta direita abre detalhe; Esc volta. Fn+Enter envia somente com editor aberto. Reboot restaura dados com editor fechado.
- Teclado UI_V2: setas e Esc sem Fn. Fn nas mesmas teclas insere vírgula/ponto/ponto e vírgula/barra/acento grave; Shift preserva símbolos. Links completos continuam digitáveis. Perfis antigos mantêm o decoder anterior.
- Mensagens enviadas verdes (`0x67f2`) e recebidas azuis (`0x65ff`), inclusive no detalhe; rótulos EU/remetente e entrega continuam presentes. Seleção e cabeçalho têm fundo distinto; aba ativa é destacada. Menu mantém o prazo original de 5 s.
- LVGL 8.3.11 fixado no perfil M7. Todas as quatro abas, editor e formulários são desenhados por objetos LVGL, com a lógica existente de navegação/persistência preservada. Fontes Adv_Plain_12/Latin-1 e geometria 240×135 mantidas. Pool de 32 labels/16 retângulos, heap LVGL estático 32 KiB e buffer RGB565 de 240×14 pixels (6.720 bytes), sem framebuffer RGB de tela inteira.
- Flush síncrono por faixas no HSPI exclusivo do TFT (pinos diferentes do LoRa/SD), preservando inicialização, inversão vertical e offsets 40/52 do driver ST7789 fixado. Sem inicialização extra do rádio ou manipulação de LVGL no worker/ISR.

A integração segue a [interface de display LVGL 8.3](https://docs.lvgl.io/8.3/porting/display.html), com flush_ready após a transferência, e o [tick monotônico](https://docs.lvgl.io/8.3/porting/tick.html). Não usa transporte externo, Wi-Fi, brilho ou NVS para dados do app.

## Validação

17 executáveis existentes com sanitizers e 10 testes Python aprovados. Mais 3 executáveis novos: renderizador real com fontes e cores/troca repetida de telas/monitor de memória LVGL; decoder de setas/Esc/Fn e todos os caracteres do link; fluxo real do Chat exigindo Enter e preservando rascunho na lista. Os mocks do shell são usados para isolar lógica/rádio; o teste de desenho compila e executa os 192 fontes C do pacote LVGL resolvido. Renderização colorida 240×135 inspecionada em preview nativo, não em fotografia do hardware.

Build 0.7.2 aprovado: aplicação **865.472 bytes**, RAM estática **142.768 bytes**, flash vinculada **865.041 bytes**. Crescimento estático de RAM em relação a 0.7.1: **40.880 bytes**. Regressão de build M6 aprovada. O usuário confirmou esta revisão funcionando no ADV. Não foram fornecidas medidas físicas de heap/latência.

[Aplicação OTA](../newversion/m7-0.7.2/cardputer-adv-0.7.2-m7-candidate-ota.bin). SHA256 `fe4c2fa8c9045df010476aded310abf83b988e8bc5d6b81a474c42f5bcf94536`. Manifesto/fontes conferidos, versões anteriores preservadas e checksums verificados. Upstream/dependências verificados; vendor sem modificações. Nenhuma gravação pelo agente, bootloader ou tabela de partições incluídos. Manifesto mantém validação física/compatibilidade instalada não verificadas.

[Operação e conferência atual](M7_UI_USAGE.md).
