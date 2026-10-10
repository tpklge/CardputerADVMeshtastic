# M5 — conversas e detalhes, candidato 0.5.0

M4 confirmado pelo usuário. M5 autorizado e implementado em 09/10/2026. Testes/build offline aprovados; recursos novos aguardam teste no ADV. Nenhum envio de firmware ou acesso ao aparelho pelo agente.

## Entrega

- Escolha de conversas dos oito slots, com canais ativos/livres e indicadores não lidos. Cabeçalho do destino coincide com canal do comando; valida identidade novamente ao executar o envio.
- Histórico filtrado por identidade lógica. Rascunhos/cursor/posição separados em pares CRC por conversa. Transição assíncrona no worker SD conserva contexto antes de mudar; envio pendente bloqueia troca até confirmação de fila/erro.
- Sequência de leitura persistida e monotônica; índice reconstruído incrementalmente no boot. RX não muda foco/aba/conversa. Página recente marca leitura, enquanto composição/páginas antigas conservam indicador.
- Lista de mensagens com remetente/nome ou ID; detalhe de texto e metadados de UTC/local, sinal, hops observados e packet ID. Fila, TX de rádio, ACK e erro continuam distintos.
- Nós com idade/SNR e três páginas: identidade/cache, rádio/hops/bateria disponível e posição com idade quando conhecida. Ausência de campos não é apresentada como um valor válido.
- GPS em duas páginas: hora UTC/local/fix/satélites; coordenadas/altitude/idade do fix. Hora GPS independe do fix posicional; sem fix não apresenta coordenadas antigas como atuais.
- Fonte e regra dos 5 segundos preservadas. M3/M4 migram sem apagar dados; schema 1 permanece. Stack do worker M5: 8 KiB. Canais editáveis/importação por link ficam em M6; DM em M7. Alerta visual implementado; beep opcional não adicionado.

## Verificação

Oito binários C++ com ASan/UBSan e oito testes Python de pacote OTA. UI real compila em framebuffer simulado 240×135, com testes de canal escolhido no TX, rascunho conservado, contexto bloqueado enquanto grava envio, RX sem roubar foco e GPS com hora válida/sem fix. Frames inspecionados em `.pio/ui-preview/m5-*.png`.

Worker SD real exercitado com filesystem/RTOS simulados: filtro de conversas, rascunhos independentes, pares de geração, CRC, watermark não regressivo, recuperação de uma geração corrompida e bloqueio/preservação quando ambas falham. Imagem de cartão é reaberta em processos novos para testar reboot, reconstrução das não lidas e contexto íntegro mais recente que o estado global após publicação interrompida. Compatibilidade de codec M3/M4/M5 coberta. Estes testes não substituem RF, FAT/controlador SD ou corte físico de energia.

Candidato [OTA 0.5.0](../newversion/m5/cardputer-adv-0.5.0-m5-candidate-ota.bin): **763.056 bytes**. SHA256: `6f191e77ca764ac4c84c1bfcad0fec2b1cb83596380b7d245609761a4809e501`. RAM estática: **87.960 bytes**; flash vinculada: **762.633 bytes**. Build de regressão M4 realizado; pacote validado 0.4.0 preservado, assim como M1–M3. Upstream continua `v2.7.26.54e0d8d`. Manifesto contém SHA das fontes e inventário. M5Launcher confirmado; versão/layout/limite instalado continuam desconhecidos, com compatibilidade instalada não verificada no manifesto.

Operação e conferência física em [M5_USAGE.md](M5_USAGE.md).
