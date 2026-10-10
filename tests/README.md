# Testes sem hardware

`tools/test_native.py` executa testes C++ com ASan/UBSan, processos adicionais para reboot e publicação interrompida e onze testes Python de OTA. Cobrem teclado, comandos/UTF-8, codecs/CRC, substituição de arquivos, editor/menu, integração de UI M4/M5 e worker real de SD com fakes de filesystem/RTOS.

A UI usa seu próprio código e fonte em framebuffer 240×135. Testa timeout 4.999/5.000 ms/rollover, RX sem roubar foco, formulários, confirmação de limpeza, TX com edição posterior e seleção de canal. Frames `.gray` em `.pio/ui-preview/` permitem inspeção.

O teste de worker prepara dados M4, alterna rascunhos de dois canais, filtra histórico, persiste/reconstrói não lidas, recupera CRC e testa erro quando ambos os slots de contexto estão inválidos. Exporta filesystem simulado em `.pio/native-storage-fixture`, reaberto em processo novo para reboot. Não usa o microSD ou aparelho real.

Neste Mac: `.venv/bin/python tools/test_native.py --sdk /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk`. Os testes não comprovam interoperabilidade RF, display físico, FAT ou recuperação após corte físico de energia.

M6 acrescenta codec/plano com protobufs fixados, fixture de oito canais, chaves Hex/Base64, preflight sem alterações, resolução de herança/duplicatas e preservação de região. UI real de canais testa criação, revisões em duas confirmações e revelação/exportação que ignora Enter repetido. Shell integrado testa canal recém-criado selecionável e destino correto. Worker M6 testa rascunho de URL no limite, reboot/CRC e exportação. A suíte inclui regressões de M4 a M8. Primeiro compilar um ambiente mesh para resolver Nanopb antes dos testes de protocolo.

A revisão 0.8.2 acrescenta fila de resultados reservados sob saturação, UTF-8 com mensagens recebidas de várias linhas, gravação de rascunho durante alterações contínuas e ausência de gravação para estado idêntico. O worker real verifica paginação nos dois sentidos. A UI verifica ordem cronológica, navegação durante composição, preservação de cursor/rascunho ao receber texto e acesso à recuperação com núcleo parado.

Checkpoint/backup usam o código de produção com SD simulado: reutilização de arquivos, falha de escrita antes do manifesto, reboot com a geração anterior, restauração sem checkpoint válido e rejeição de MAC de outro aparelho mesmo com CRCs corretos. O teste de manutenção executa o worker real e verifica que somente a restauração em recuperação dispensa núcleo/histórico e flush. Esses fakes não modelam concorrência ou temporização física do FreeRTOS/SPI.

`tools/test_lvgl.py` compila o LVGL fixado e verifica que repetir uma tela idêntica não provoca outro flush; mudanças de texto e trocas de conteúdo/tema atualizam o framebuffer. Inclui também teclado e integração do editor.

1.0.0 acrescenta o caso `--continuous-chat`: percorre mensagens enviadas/recebidas uma a uma, verifica a sobreposição das linhas ao rolar nos dois sentidos, mantém a posição nos extremos e ao receber uma mensagem durante leitura de histórico antigo, preserva rascunho/cursor e abre o detalhe correto. A suíte de empacotamento tem 12 casos e mantém a indicação de contrato instalado não verificado na publicação autorizada.
