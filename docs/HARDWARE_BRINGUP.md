# Diagnóstico M1 no ADV

O usuário instalou pelo launcher e confirmou boot, leitura básica do teclado, orientação corrigida, teste de microSD, hora GPS e Cap indicado como OK em 09/10/2026. Relatou ausência de TX/RX e zero erros. O agente não acessou o dispositivo ou enviou firmware. Fix de posição, recepção RF e coexistência RX/SD continuam pendentes; TX está desabilitado neste diagnóstico.

## Antes de instalar futuramente

Usar exclusivamente o fluxo OTA já existente do **M5Launcher**. Verificar versão/layout e que o slot de aplicação comporta o tamanho exato informado em `newversion/m1/manifest.json`. A tabela gerada para linkedição não deve ser instalada. O pacote contém aplicação, checksum e manifesto; não inclui bootloader/partições.

M1 não entra na rede Meshtastic e não transmite. A recepção usa um perfil fixo de bancada **915 MHz / BW 250 kHz / SF11 / CR5 / sync word 0x2b / preâmbulo 16**; não é configuração regional/canal do usuário. Ausência de pacotes nessa frequência não comprova falha de rádio. Para coexistência RX/SD, o emissor de teste precisa usar perfil compatível; tráfego mesh real será validado após M2.

## Tela e operações

Depois de iniciar, a tela mostra Cap/SX1262, contagem de RX/erros, bateria em volts, UTC/fix/satélites ou NMEA/checksum, SD, tempos máximos SPI, última tecla e heap/loop.

- **Enter**: iniciar teste SD ou tentar remontar após ausência/falha. Escreve 4 KiB em blocos de 256 bytes e relê todos os bytes.
- **Shift mantido + letra/símbolo**: testar mapa alternativo. `:`/`?`/`#`/`-`/`_`/`=`/`/` precisam ser digitáveis para os links.
- **Tab / Shift+Tab**: aparecem como eventos lógicos na linha do teclado. Ainda não há abas/menu de produto.
- **Fn mantido + `, ; . /`**: esquerda/cima/baixo/direita.
- **Fn + tecla de crase/Esc**: Escape; **Fn+Enter**: ação Enviar apenas exibida neste diagnóstico.
- Segurar letra/Backspace/seta: início da repetição em 450 ms, depois intervalos mínimos de 80 ms. Tab, Enter e Enviar não repetem.

Saídas no cartão:

```text
/CardputerADVMeshtastic/diagnostics/m1-spi-probe.bin
/CardputerADVMeshtastic/diagnostics/m1-report.json
```

São arquivos exclusivos de diagnóstico, sobrescritos no próximo teste. Não são banco de dados final. Escrita só começa por Enter; montagem inicial não formata o cartão. Erros continuam visíveis em tela. Flush de FAT/latência de SD não são garantidos por usar blocos pequenos: medir tempos reais.

## Sequência de validação pendente

1. Boot frio com Cap/SD: confirmar orientação e conteúdo integral da tela, teclado e bateria plausível.
2. Teclado: exercitar todos os símbolos de URL, combinações e soltura; conferir que Fn+B não ativa Bluetooth e Fn+Q não reinicia.
3. GPS: ao ar livre, observar NMEA, UTC, satélites e fix separadamente. Sem fix, UTC pode estar válido; sem UTC recente, mostrar `--:--`.
4. SD: executar Enter e conferir `Leitura/escrita 4KiB OK`; ler relatório/arquivo posteriormente se desejado.
5. RX de bancada durante teste SD: observar recepção, CRC/erros e tempos de espera/posse do SPI. Repetir Enter e comparar com recepção sem escrita.
6. Boot sem cartão, cartão cheio/somente leitura, remoção durante teste e reinserção: garantir erro visível, UI funcional e ausência de formatação.
7. Depois de coleta suficiente, registrar valores reais em `M0_M1_STATUS.md`. Não marcar RX/TX mesh aprovado com base neste perfil bruto de recepção.

M1 tem apenas RX; teste de TX/roteamento com o SD em atividade depende do núcleo local em M2. Não há botão de retorno ao launcher até verificar seu mecanismo instalado; encerrar/reiniciar pelo procedimento físico já utilizado pelo usuário.

## NVS técnico

Arduino-ESP32 2.0.17 tenta apagar NVS automaticamente quando a inicialização encontra certos erros. O build M1 envolve `esp_partition_erase_range` para recusar apagamento da partição NVS. Se esse caminho for acionado, o diagnóstico indica `NVS preservado`; não tenta recuperar formatando. Outros apagamentos não são solicitados pela aplicação M1.

Não há armazenamento de identidade/canais/histórico em flash neste estágio. Integração do storage operacional do núcleo com SD pertence a M3.
