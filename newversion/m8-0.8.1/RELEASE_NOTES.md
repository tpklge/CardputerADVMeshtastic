# Ajustes de interface — 0.8.1

O usuário confirmou 0.8.0 funcionando no ADV e solicitou estes ajustes.

- Barra superior sem fundo destacado, oculta no boot e exibida com Tab, junto do menu inferior. Ambos somem após cinco segundos; digitação/RX não renovam o prazo. Abrir DM com menu oculto não o reabre.
- RF/SD/hora à esquerda; somente porcentagem estimada de bateria à direita, substituindo a tensão. Usa a estimativa calculada pelo núcleo oficial a partir da leitura da placa. Ausência de leitura válida aparece como `--%`, sem inventar carga.
- Durante a composição, histórico continua visível e o rascunho aparece somente no editor inferior. Removida a cópia do texto no centro; contador/atalho de envio continuam acima do editor.
- Temas, persistência, canais, DM, traceroute e backups mantidos. Schema não alterado. Pacote 0.8.0 preservado.

Build alvo e suíte nativa aprovados: 20 executáveis C++ com ASan/UBSan, incluindo novos casos de porcentagem 0/75/100/desconhecida, alinhamento, barra no boot/Tab/timeout/DM, histórico durante a composição e texto em uma única linha de edição com menu aberto/oculto. Onze testes Python de OTA aprovados. Vendor inalterado; hashes das 11 aplicações preservadas conferidos.

Aplicação: `cardputer-adv-0.8.1-m8-candidate-ota.bin`; **878784 bytes**. RAM estática **145872 bytes**, flash vinculada **878361 bytes**. SHA-256 `c7bfdcf6ad70526ea5556385f3c70d0ccc324979f5a72f4085b11e95580e5e51`. Manifesto e hash das fontes conferidos.

Somente aplicação OTA; não houve upload/gravação. A revisão 0.8.1 ainda requer conferência no ADV; o manifesto mantém o estado de candidato e o contrato instalado não verificado. Operação: [M8_USAGE.md](../../docs/M8_USAGE.md).
