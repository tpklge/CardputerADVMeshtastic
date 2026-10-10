# M3 — uso do candidato 0.3.0

Instalar somente a aplicação OTA pelo M5Launcher já existente, mantendo o mesmo microSD e os dados do M2. O agente não envia firmware nem altera partições.

A fonte nominal passa de 10 para 12 pixels, com raster de 16 pixels. A tela de status foi compactada para acomodar o texto maior. Esta é a interface provisória; o menu final com timeout de cinco segundos pertence ao M4.

## Teste inicial

1. Após instalar, verificar `M3 pronto` e SD OK (ou REC quando houve recuperação).
2. Conferir identidade, região, preset e canais existentes. Não é necessário redefinir BR_902 ou qualquer região já escolhida.
3. Digitar um rascunho, esperar pelo menos dois segundos e reiniciar. O texto e cursor devem reaparecer.
4. Enviar texto com Fn+Enter e receber outra mensagem.
5. Usar Tab até `Historico SD`. Reiniciar e conferir as duas mensagens nessa tela.
6. Conferir os nós recebidos após pelo menos dois segundos de funcionamento e novo boot.

## Teclas e preferências

- Tab / Shift+Tab: status, nós, ajuda, histórico.
- Fn+setas cima/baixo no histórico: mensagem anterior/seguinte dentro da página; cima na primeira pede mensagens mais antigas, baixo na última retorna às mais recentes.
- Fn+setas esquerda/direita no histórico: trechos anteriores/seguintes do texto completo, duas linhas por trecho.
- Fora do histórico, Fn+esquerda/direita movem o cursor; Fn+cima/baixo paginam nós.
- Enter executa os comandos do M2; Fn+Enter envia texto.
- `/sd`: mostra registros, saturações, observações de CRC inválido e registros retirados por retenção.
- `/utc -240`: salva offset em minutos (intervalo −720 a +840). A configuração é persistida; esta tela de diagnóstico continua mostrando UTC explicitamente. A apresentação de hora local será integrada à UI do M5.

O histórico distingue fila/resultado desconhecido, TX no rádio, ACK/NAK quando recebido e falha. TX no rádio não comprova entrega. Mensagens sem hora GPS válida mostram hora desconhecida; ordem é dada pela sequência persistente.

## Falhas e retenção

SD ERRO bloqueia novos envios e alterações que exijam persistência. Quando a configuração falha durante uma sessão já iniciada, RX continua em RAM; os dados recebidos após a falha podem não ser salvos. Reinserção exige reiniciar, sem remontagem automática.

O histórico usa 96 segmentos de até 512 registros: menos de 19 MiB e no máximo 49.152 registros (incluindo estados, posições e métricas). Ao atingir esse limite, o segmento mais antigo é retirado. REC sinaliza registro incompleto/corrompido; FILA! sinaliza saturação da fila de persistência. Não há formato automático nem fallback NVS.

Não testar cortes de energia durante escrita em cartão que contenha dados insubstituíveis. Os testes locais exercitam o formato e a seleção de snapshots; a resistência do FAT/controlador a interrupção física ainda requer validação no ADV.
