# board

M1 implementa teclado/decoder, Cap/SX1262 RX, GPS, bateria, worker SD e SPI com lock upstream e métricas. `preserve_nvs.cpp` bloqueia autoerase do NVS técnico. Display do diagnóstico está em `src/ui/diagnostics.cpp`. M2 exclui `board.cpp` e transfere rádio/GPS ao núcleo, preservando o diagnóstico M1; não manter instâncias concorrentes nos mesmos sinais.
