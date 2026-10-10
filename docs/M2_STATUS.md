## Correção 0.2.1-m2

Relato de hardware: configuração BR_902/hops 3, SD erro, configuração incompleta e núcleo parado. Corrigida incompatibilidade potencial entre troca de arquivos do núcleo (LittleFS) e FAT no SD. Agora usa backup durante substituição; boot valida protobuf/versão e recupera temporários/backups quando há cópia válida. Remanescentes são preservados em `.saved-N`. Não há recuperação de um conjunto parcialmente inexistente sem cópia válida; não se infere nem recria configuração perdida.

Teste nativo com FAT simulada verifica sobrescrita, primeiro arquivo, falha ao guardar original, falha ao promover temporário com restauração e recusa de backup pendente. Build 0.2.1 aprovado; aplicação OTA com 712.576 bytes. Testes nativos com ASan/UBSan e cinco testes Python de empacotamento aprovados. Em 09/10/2026, o usuário confirmou que a configuração permanece, conseguiu alterar a região para ANZ e enviou e recebeu mensagens no ADV com 0.2.1. Isso valida a persistência observada e RX/TX básico; não comprova ACK, retries, carga prolongada ou recuperação sob corte de energia. Os números abaixo descrevem o candidato 0.2.0 anterior; consulte o manifesto do 0.2.1 para o novo tamanho/checksum.

# M2 — núcleo local e candidato 0.2.0-m2

Entrega de software em 09/10/2026. Build offline e testes nativos aprovados; nenhuma instalação realizada pelo agente. Boot, persistência observada, alteração local de região e envio/recepção de mensagens foram posteriormente confirmados pelo usuário no 0.2.1.

## Entrega

- Núcleo oficial fixado: `v2.7.26.54e0d8d`, com `ReliableRouter`, canais, criptografia simétrica/PKI, deduplicação e retransmissões oficiais.
- Único SX1262 e única UART GPS pertencem ao núcleo. Teclado TCA8418, decoder testado e display ST7789 pertencem à aplicação. Drivers de bancada do M1 não entram no build M2.
- Adaptador tipado em `mesh_backend.hpp`: comandos de texto para canal/nó, nome e LoRa; eventos copiados de texto, posição, métricas de dispositivo, nó, configuração e feedback.
- Fila de seis comandos e dezesseis eventos, protegida para o callback do worker de rádio; rejeição de comandos cheios/inválidos e contagem de eventos descartados. O consumidor não recebe ponteiros a pacotes liberados pelo núcleo.
- `Queued` significa aceitação local; erros da fila RF vêm de QueueStatus. `Transmitted` vem do tratamento de TX_DONE, antes da liberação do pacote. ACK/NAK vem de Routing com request ID; não criar retries paralelos nem afirmar entrega a partir de TX_DONE.
- Consulta de nós em páginas de até quatro registros; diferencia local, cache e ouvido neste boot. Rastreamento limitado a 200 IDs; após saturação, registros sem evidência mostram `RX ?`, em vez de serem classificados falsamente como cache.
- Validação de bytes/UTF-8, nomes não vazios, canal, região compatível, preset, hops e slot dentro da faixa efetiva da região/preset. Limite de texto M2: 200 bytes; textos maiores não são truncados silenciosamente.
- Região inicial `UNSET`; escolha local explícita habilita TX. Presets, canal primário e limites de rádio seguem o núcleo. Não repetir a frequência fixa de bancada do M1.
- Wi-Fi, BLE, MQTT, webserver, console serial externo, áudio, sensores externos e UI oficial excluídos. Administração remota também excluída neste candidato; configuração básica é local.

## Overlay

`tools/stage_core.py` gera `.pio/core-m2` a partir dos fontes rastreados no checkout fixado. Âncoras de alteração precisam corresponder exatamente; mismatch interrompe o build. O checkout original permanece sem alterações.

As alterações renomeiam setup/loop para integração, limitam o atraso do loop externo, redirecionam FSCom ao SD, adaptam mutexes FreeRTOS para recursão dos wrappers FS, removem uso de Preferences para metadados do app e recusam factory reset/autoerase. O SPI já iniciado pelo produto é reutilizado pelo núcleo. A variante de áudio upstream não é compilada; o expansor do Cap é inicializado pelo produto conforme registradores oficiais.

## Armazenamento mínimo necessário a M2

Os arquivos do núcleo ficam exclusivamente em:

```text
/CardputerADVMeshtastic/core/prefs/device.proto
/CardputerADVMeshtastic/core/prefs/config.proto
/CardputerADVMeshtastic/core/prefs/module.proto
/CardputerADVMeshtastic/core/prefs/channels.proto
/CardputerADVMeshtastic/core/prefs/nodes.proto
```

Todos os acessos FS/File do adaptador compartilham `spiLock` com RadioLib. Montagem não formata. Primeiro boot exige cartão montado e gravável; configuração totalmente ausente permite criação inicial. Conjunto parcialmente presente, arquivos críticos `.tmp`, versão incompatível ou falha de decode bloqueiam o núcleo e conservam os arquivos. Depois do boot, uma checagem de leitura a cada dois segundos e falhas de I/O detectadas travam gravações; o loop interrompe o núcleo e desabilita o rádio ao perceber a falha. Isso não garante detecção instantânea de toda remoção física.

`SafeFile` mantém o arquivo anterior durante escrita temporária; falha de close/readback passa a ser retornada corretamente por `NodeDB::saveProto`. Readback upstream ainda usa XOR, e substituição FAT não é uma transação entre todos os arquivos. Não afirmar recuperação de corte de energia ou atomicidade total. O M3 implementará gerações, CRC, recuperação, histórico, rascunhos e persistência completa do produto. Algumas operações upstream ainda seguram o lock por leituras inteiras; latência/coexistência real permanece a medir e melhorar em M3.

Não apagar automaticamente arquivos para reparar um bloqueio. Conservar os arquivos e avaliar a causa antes de qualquer recuperação manual.

## Validação executada

- Cross-build ESP32-S3 e empacotamento de aplicação com descritor, checksum e SHA-256 embutido verificados.
- Teclado e fronteira mesh em C++ com AddressSanitizer/UndefinedBehaviorSanitizer: fila cheia/vazia, FIFO/ownership de cópias, limites de texto/nome, UTF-8 inválido e parâmetros inválidos.
- Cinco testes Python do pacote: inclui manifesto M2 com runtime mesh habilitado e ausência de afirmação de hardware/slot validado.
- Checkout/submódulo requerido conferidos contra lock.
- Regressão de build M1 aprovada após os ajustes compartilhados; seu pacote já testado pelo usuário não foi substituído.
- Inspeção offline por esptool: checksum/hash válidos; símbolos de ReliableRouter, criptografia e proteção NVS presentes, sem inicialização LittleFS/Preferences no ELF.

Candidato atual: **707.888 bytes**, RAM estática **71.616 bytes** e flash de linkedição **707.453 bytes**. Esses valores não representam heap livre em execução nem o limite do slot instalado. Tamanho e hash exatos do candidato estão em `newversion/m2/manifest.json` e `SHA256SUMS.txt`. Uso de RAM estática consta no log; heap, stack, latência e drops no ADV ainda não medidos. O build usa `default_8MB.csv` apenas como referência; não muda partições instaladas.

## Pendências para concluir M2

Boot frio com cartão, rádio inicializado, hora/fix pelo GPS do núcleo, nomes/região/preset persistidos após reinício, RX/TX com outro nó compatível, feedback de envio e coexistência rádio/SD. O relato mais recente confirma envio e recepção de mensagens, superando a limitação anterior de ausência de emissor. Ainda faltam evidências específicas de dados de nó/posição, feedback ACK/NAK, persistência de nome/preset, boot frio e comportamento sob carga/falhas; o binário permanece candidato de integração.

Procedimento de operação em [M2_USAGE.md](M2_USAGE.md). M1 aprovado pelo usuário continua disponível em `newversion/m1`.
