# Publicação 1.0.0 e registro de M8

O usuário confirmou 0.8.2 funcionando e autorizou a publicação de 1.0.0 em `main`, com binários e licença GPL-3.0 existente. O histórico final usa uma lista contínua com rolagem por mensagem. Consulte [notas da publicação](../newversion/1.0.0/RELEASE_NOTES.md) e [operação atual](M8_USAGE.md). As informações abaixo são registros históricos; não representam confirmação física da alteração final.

# M8 — Consolidação 0.8.0

Correções posteriores: candidato [0.8.2](../newversion/m8-0.8.2/RELEASE_NOTES.md), com recuperação, resultados reservados, gravação periódica, histórico e atualização LVGL. A revisão 0.8.2 não tem validação física registrada.

Atualização: o usuário confirmou 0.8.0 funcionando antes de solicitar os ajustes de [0.8.1](../newversion/m8-0.8.1/RELEASE_NOTES.md). Os resultados abaixo registram a preparação original de 0.8.0; flags do manifesto são as emitidas antes dessa confirmação. Não foram fornecidas medidas físicas de heap/latência.

Em 10/10/2026, o usuário confirmou os ajustes de 0.7.2 no ADV e autorizou terminar o desenvolvimento, acrescentando temas de cores. M7 agora tem os quatro grupos implementados: DM PKI; busca/favoritos/detalhes; traceroute manual; diagnóstico e backup/restauração. A consolidação offline de M8 e o pacote candidato estão documentados aqui. A aceitação física dos novos recursos e a verificação do limite/layout do launcher instalado permanecem abertas; esta entrega não afirma release homologado.

## Implementação

- Quatro temas com contraste mínimo medido de **7,84:1** entre texto (inclusive envio/recepção) e fundo normal/seleção/cabeçalho; aba ativa também passa 7:1. LVGL, fonte, teclas, abas e timeout preservados.
- Preferência global no microSD/schema 1, byte reservado 372. A seleção de novos ajustes usa byte 373; o campo anterior continua limitado a 11, permitindo que leitores antigos ignorem a extensão sem rejeitar os dados. Registros anteriores selecionam Escuro. Não há conversão/remoção da base.
- Traceroute via `TraceRouteModule::startTraceRoute`: adapter de leitura do estado/resultado/cooldown no overlay, sem modificar o checkout vendor nem implementar protocolo paralelo. Resultado paginado/capturado, timeout, limites e exportação manual no SD.
- Manutenção em worker de 8 KiB, uma operação por vez. Backup imutável de checkpoint, CRC/readback e validação protobuf/version/core/ID local. Decodificação dos nós limitada a 200 entradas, sem criar um vetor adicional durante a validação. Manifesto é publicado após a cópia dos arquivos.
- Restauração escreve a geração inativa, publica um novo manifesto e valida; somente então bloqueia novas gravações do núcleo e reinicia. A inicialização normal restaura essa geração. Não altera os arquivos de mensagens, rascunhos ou preferências. O rascunho passa por flush/readback antes de iniciar.
- Diagnóstico por lista explícita de campos, sem objetos de configuração/keys/mensagens. Acrescentados heap livre, uptime e duração máxima do loop para a medição no aparelho. Duração de manutenção apresentada ao concluir.
- Perfil padrão `cardputer-adv-m8`; versão `0.8.0-m8`; pacote de aplicação sem bootloader/partições. Perfis e pacotes anteriores preservados.

## Verificações

`tools/test_native.py`: **20 executáveis C++** com AddressSanitizer/UndefinedBehaviorSanitizer e **11 testes Python**. Inclui shell real de temas/prévia/cancelamento/SD com falha, confirmação de backup/restauração, traceroute/resultado/erro/exportação; contraste RGB565; codec e compatibilidade; persistência de tema através de troca de conversa e reboot em processo novo.

O teste de core compila as funções reais de checkpoint/backup, substituindo somente o adapter virtual do filesystem por SD com injeção de falhas. Verifica cópia íntegra, ID incorreto, manifesto corrompido, cópia interrompida sem reutilizar nome, restauração com falha durante arquivo e durante manifesto, seleção da última geração válida no reboot, restauração concluída e preservação de históricos/rascunhos. Não é um ensaio de desligamento físico/FAT no cartão real.

`tools/test_lvgl.py`: **3 executáveis adicionais**, incluindo os 192 fontes C do LVGL fixado, renderização dos quatro temas, cores/fonte, troca repetida de tela e heap LVGL livre acima de 4 KiB. Previews nativos 240×135 inspecionados; não são fotografias do ADV. Teclado e fluxo Enter antes da composição continuam passando.

Build alvo M8 e regressão M6; fontes/submódulos fixados; dependências Python sem conflitos; manifests/checksums dos pacotes anteriores conferidos. O candidato tem **878,624 bytes**, RAM estática **145,872 bytes** e flash vinculada **878,193 bytes**. SHA-256 `0fbb67951e1d96b5ea42ad44c227aff11a97688cd52736b28d2abdf0d15f8c4e`. Recursos e hashes também estão no [manifesto](../newversion/m8/manifest.json) e nas [notas de entrega](../newversion/m8/RELEASE_NOTES.md).

## Limites de validação

O usuário confirmou 0.7.2 funcionando no aparelho. Os novos temas, traceroute e manutenção de 0.8.0 passaram por build/testes offline; precisam da conferência descrita em [M8_USAGE.md](M8_USAGE.md). Latência máxima e heap durante rádio/SD dependem do cartão e do tráfego reais; o novo diagnóstico fornece as métricas, sem inventar valores físicos.

M5Launcher está confirmado, mas versão/layout/tamanho aceito do app slot não foram identificados. O manifesto mantém `hardware_validated=false` e `installation_compatibility_verified=false` para este candidato. O layout de referência usado na linkedição não altera a instalação. Nenhum acesso de gravação ao aparelho foi realizado.
