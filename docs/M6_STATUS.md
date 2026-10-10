# M6 — canais standalone, candidato 0.6.0

Autorizado pelo pedido de continuar após M5. O usuário relatou que apenas LongFast estava disponível e os canais eram somente leitura; M6 permite criar/importar secundários e selecioná-los no Chat. Build/testes offline aprovados; validação física das alterações pendente.

## Implementação

- Ações nos oito slots: criar secundário, editar nome/PSK, gerar AES256 com RNG do ESP32, desativar secundário, importar link e exportar canal/conjunto. Campos separados e confirmações antes de alterar.
- Codec do Tab5 adaptado ao protobuf/Nanopb fixado do ADV. Links completos digitáveis, Base64 URL-safe, UTF-8, PSKs e limites validados. Não acessa rede. Keys editáveis em Hex/Base64 padrão.
- Importar adicionar/substituir com preflight completo, duplicatas por nome resolvido/chave efetiva, herança contra origem e rejeição sem mudanças quando faltam slots. Substituir conserva restrições locais; LoRa opcional aceita presets sem override/offset RF. Configuração customizada é rejeitada quando solicitada para aplicar.
- Transação assíncrona no backend local com revisão da configuração, rádio sem TX/fila em andamento e snapshot/readback SD antes de aplicação. Falha de persistência conserva estado anterior em RAM e sinaliza SD; falha posterior de rádio indica resultado sem declarar sucesso.
- Cache de canais/identidades/índice do Chat atualizado após commit. Rascunho antigo permanece ligado à identidade anterior; não é enviado pelo reaproveitamento de slot. Teste integrado comprova novo terceiro canal selecionável e destino correto no comando de envio.
- Rascunho de URL/cursor em pares versionados CRC no SD, até 2.047 bytes; pausa de 1 s antes do flush. Arquivo exportado contém chave efetiva, exige confirmação e é relido antes de indicar sucesso. Sem câmera/Wi-Fi/brilho/transporte externo.
- Stack do loop M6: 16 KiB para codec/revisão/snapshot; worker SD: 8 KiB. Fonte/menu de 5 s preservados.

## Verificação

Doze binários C++ com sanitizers: codec/plano, UI de canais, integração do shell após mudança de canais, worker M6 e oito casos existentes. Nove testes Python de empacotamento. Worker executado em processos novos para reboot, publicação interrompida e URL com duas gerações corrompidas. Persistência de 2.047 bytes/cursor, exportação e CRC/readback cobertos. Codec usa protobufs oficiais da revisão fixada e fixture de oito canais serializada independentemente no Tab5; nenhum checkout do Tab5 foi modificado.

Renderização real da fonte em framebuffer 240×135 inspecionada para revisão/importação/exportação. Build M6 aprovado: aplicação **775.872 bytes**. RAM estática **96.352 bytes**. flash vinculada **775.437 bytes**. Build de regressão M5 realizado sem reempacotar versões anteriores.

[Aplicação OTA](../newversion/m6/cardputer-adv-0.6.0-m6-candidate-ota.bin). SHA256 `b7b894cf5deec763df1b31e0879c087f3257096830b0637794beea33550b40e3`. Upstream permanece `v2.7.26.54e0d8d`; schema de eventos/configuração continua 1 e rascunho de URL usa formato URL6 versão 1. Manifesto contém hash das fontes/inventário; layout/limite instalado continuam não verificados. Nenhum firmware enviado, bootloader ou partições incluídos. Pacotes M1–M5 preservados.

Interoperabilidade com nó externo, SD físico/corte de energia e comportamento da interface no ADV ainda precisam de teste. Este candidato implementa o marco; não declara o MVP validado fisicamente. [Uso e conferência](M6_USAGE.md).

## Retorno no ADV

O usuário confirmou “tudo certo, podemos passar para o próximo passo” após M6. Este retorno autoriza avançar para M7 e registra aprovação de uso no aparelho; não substitui ensaio destrutivo de corte de energia nem informa tamanho/layout do slot do launcher.
