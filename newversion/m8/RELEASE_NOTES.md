# Candidato OTA M8 — 0.8.0

Consolidação offline de 10/10/2026, após confirmação de 0.7.2 no ADV pelo usuário.

- Temas Escuro, Noite âmbar, Claro e Preto alto contraste, em AJUSTES → Tema de cores, com prévia/cancelamento e persistência global no microSD.
- Traceroute manual: T no detalhe de nó; setas rolam; E exporta rota no SD; timeout/cooldown do núcleo.
- AJUSTES → Backup / exportar: diagnóstico sem chaves/mensagens; backup de configuração; seleção/restauração compatível com reinício. Preserva histórico, rascunhos e tema.
- UI LVGL, Enter antes de compor, setas/Esc sem Fn e menu de cinco segundos preservados.

Aplicação: `cardputer-adv-0.8.0-m8-candidate-ota.bin`; **878,624 bytes**. RAM estática: **145,872 bytes**. Flash vinculada: **878,193 bytes**. Worker de manutenção: 8 KiB; heap LVGL: 32 KiB estático. Crescimento do app frente a 0.7.2: 13,152 bytes. Valores de RAM não incluem todo o heap/stacks dinâmicos; diagnóstico no ADV mostra heap livre/mínimo e maior duração do loop.

SHA-256: `0fbb67951e1d96b5ea42ad44c227aff11a97688cd52736b28d2abdf0d15f8c4e`.

23 executáveis C++ (20 da suíte principal + 3 de LVGL/teclado/chat), ASan/UBSan, 11 testes Python, build M8 e regressão M6 aprovados. Recuperação de checkpoint/backup testada com injeção de escrita parcial no SD simulado. Previews LVGL das quatro paletas inspecionados. Fontes e hashes conferidos; pacotes anteriores preservados.

Este é um **candidato**, com novos recursos ainda sem validação física. M5Launcher confirmado; versão/layout/limite do slot instalado permanecem desconhecidos. Manifesto mantém hardware/compatibilidade instalada não verificados. Somente aplicação: não contém bootloader/tabela de partições e não foi enviada ao aparelho.

Operação e conferência: [M8_USAGE.md](../../docs/M8_USAGE.md). Evidência/limites: [M8_STATUS.md](../../docs/M8_STATUS.md).
