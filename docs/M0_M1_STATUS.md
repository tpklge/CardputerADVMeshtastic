# Estado de M0 e M1

Atualizado em 09/10/2026. Entrega de software preparada; nenhum dispositivo foi acessado ou firmware enviado.

Relato do usuário em 09/10/2026: candidato 0.1.0-m1 abriu no ADV pelo launcher e leu o teclado corretamente. Tela estava invertida; 0.1.1-m1 aplica rotação de 180° pelo driver ST7789, ainda aguardando confirmação visual. Isso confirma boot e leitura básica do teclado, sem validar os demais periféricos ou o contrato completo do launcher.

Confirmação posterior do usuário, na mesma data: orientação corrigida na 0.1.1-m1 e teste de microSD aprovados. Registrado como validação funcional de escrita/leitura do diagnóstico; ainda sem valores de latência, teste simultâneo de RX ou testes de falha do cartão.

Novo relato do usuário em 09/10/2026: GPS recebeu hora, Cap indicado como OK, sem TX/RX e zero erros. Hora GPS validada; posteriormente o usuário informou `fix -- / sat 0` e ausência de outro emissor para teste. Fix de posição e RF continuam pendentes. Cap OK confirma inicialização do expansor, sem comprovar recepção RF. Zero erros corresponde ao contador observado, sem tráfego recebido; TX está desabilitado neste diagnóstico e RX permanece pendente com emissor compatível.

## M0

| Item | Evidência / estado |
|---|---|
| Base escolhida | Meshtastic Beta `v2.7.26.54e0d8d`, SHA `54e0d8d0ab2ff56b3a9ce967e53f79e49af560fb` |
| Protobufs/submódulos | Gitlinks fixados no lock; `protobufs` requerido, `meshtestic` fixado mas não necessário no diagnóstico |
| Integração | Overlay de build próprio sem modificar checkout; quatro fontes upstream de hardware/concorrência compilados diretamente |
| Toolchain | PlatformIO 6.2.0, Espressif32 6.13.0, Arduino-ESP32 2.0.17, Xtensa 8.4.0; versões no lock/manifesto |
| Recursos excluídos | Sem inicialização de Wi-Fi, Bluetooth, MQTT, HTTP ou nó completo; flags de exclusão mantidos |
| Build offline | Compilação de aplicação M1 concluída; sem alvo de upload no fluxo autorizado |
| Pacote OTA | Aplicação ESP32-S3 validada por estrutura/descritor/checksum/hash; checksum e manifesto gerados |
| Instalador | Usuário confirmou Launcher do desenvolvedor do launcher Tab5: M5Launcher/bmorcelli |
| Compatibilidade instalada | **Pendente** versão, layout e tamanho do slot do ADV; manifesto não a afirma |
| Retorno/recuperação | **Pendente** comportamento do instalador instalado; sem botão de retorno presumido |

O layout `default_8MB.csv` do board é somente referência de compilação. O binário de aplicação não substitui a tabela instalada. A referência de M5Launcher 2.9.1 consultada publica uma tabela inicial mínima, não um limite que permita inferir todos os layouts de usuário. Assim, não converter flash total ou CSV publicado em tamanho de slot confirmado.

## M1

| Item | Implementação | Validação |
|---|---|---|
| Display ST7789 | Driver fixado, buffer pequeno e diagnóstico 240×135 | Orientação corrigida confirmada pelo usuário no ADV |
| Teclado TCA8418 | Base upstream + decoder próprio de eventos/combinações/repetição; recuperação de overflow | Testes nativos com ASan/UBSan aprovados; leitura básica confirmada pelo usuário; combinações/repetição pendentes no aparelho |
| Cap/switch RF | I²C 0x43, configuração conforme variante | Cap OK confirmado pelo usuário; funcionamento RF pendente |
| SX1262 | RadioLib com HAL que participa de `spiLock`; RX bruto 915 MHz, sem TX | Compila; recepção física pendente |
| GPS | UART1 RX15/TX13, 115200, TinyGPSPlus, validade separada de hora/fix | Hora recebida confirmada pelo usuário; fix de posição pendente |
| Bateria | ADC10, tensão pelo divisor da variante | Compila; confronto com leitura física pendente |
| SD | Worker, CS12, montagem sem formato, teste explícito de 4 KiB por blocos com readback | Teste funcional aprovado pelo usuário no ADV; coexistência com RX e falhas físicas pendentes |
| Concorrência | SPI único com lock upstream; HAL/worker compartilham disciplina; ISR só sinaliza | Revisão estática; latências/drops reais pendentes |
| Diagnóstico | Tela + relatório SD: erros, heap, SPI e loop inteiro incluindo display/delay | Compila; medições reais pendentes |
| NVS do instalador | Recusa autoerase do startup Arduino na partição NVS | Wrapper presente no binário; caminho físico pendente |

## Verificações executadas

- Build cross-compilado do ambiente `cardputer-adv-m1` e inspeção offline da imagem de aplicação.
- Testes C++ de teclado: modifiers mantidos/soltos, Fn+Enter/setas, Shift+Tab, símbolos de link, repetição, ações sem repetição, entradas inválidas e rollover uint32; AddressSanitizer/UndefinedBehaviorSanitizer.
- Quatro testes Python do empacotador: imagem válida, chip/bootloader/merged inválidos, corrupção/truncamento e gates de contrato/tamanho antes de criar saída.
- Revisão do checkout/submódulo requerido contra lock e checagem das dependências Python.

O ambiente local usa Python 3.11 em `.venv`. SDK 26.5 foi selecionado para os testes nativos por incompatibilidade do linker instalado com o SDK padrão 27. Isso não altera o SDK/toolchain de compilação ESP32 nem configurações globais do sistema.

Tamanho exato, checksum, inventário resolvido e hash dos fontes do candidato atual estão em `newversion/m1/manifest.json` e `SHA256SUMS.txt`. RAM estática e uso de flash da linkedição estão no log do build; heap/latência no aparelho não foram medidos.

## Mapa para a próxima integração

Na base fixada, `src/main.cpp` inicializa `spiLock` em setup, tem chamada genérica de `setupSDCard` e inicializa SPI do rádio mais adiante; a variante ADV original não define o SD do produto. **Não habilitar a chamada genérica e repetir `SPI.begin` sobre o barramento em uso.** Integrar o proprietário de SPI antes das duas inicializações em M2/M3.

`src/graphics/Screen.cpp` usa ST7789Spi; `src/input/CardputerKeyboard.cpp` fornece a matriz, mas está acoplado ao InputBroker/UI original. O decoder próprio é independente desse dispatcher. `src/FSCommon.h` e `NodeDB` usam LittleFS: M3 deve adaptar load/save de toda a configuração ao SD, não apenas acrescentar um log.

O diagnóstico não compila/inicia o runtime completo de mesh. Sua inicialização é deliberadamente restrita ao hardware M1. M2 deve transferir ownership dos periféricos ao núcleo sem manter dois SX1262, duas UART GPS ou dois teclados ativos.

## Conclusão dos marcos

M0 está pronto quanto a fontes/ferramentas/build e pacote candidato; falta confirmar contrato **instalado** de OTA/retorno. M1 está implementado em software; critérios de boot frio, coexistência física e GPS real permanecem pendentes. Não declarar ambos integralmente concluídos antes dessas evidências.
