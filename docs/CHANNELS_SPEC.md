# Canais e importação de links

Implementação M6: [status](M6_STATUS.md) e [uso](M6_USAGE.md). A base fixada confirmou oito slots, nome até 11 bytes e PSK de 0/1/16/32 bytes. Importação aplica opcionalmente presets; LoRa customizado/override/offset não são aplicados. Leitura de `.url` permanece futura.

## Configuração local

Trabalhar com os slots do núcleo Meshtastic fixado; expectativa inicial de oito slots, um primário e até sete secundários. O canal de mensagens não é uma frequência RF independente: preset e parâmetros LoRa são comuns ao rádio. Mudança de canal primário/LoRa pode afetar comunicação e precisa de revisão própria.

MVP:

- Ver slots, nome, primário/secundário/desativado e tipo de chave; chave mascarada por padrão.
- Criar secundário em slot livre, editar nome/PSK, gerar AES-256 e desativar secundário.
- Alterar conjunto/primário através de importação/substituição revisada.
- Validar nome conforme limites do protobuf escolhido, por bytes UTF-8. No Tab5 a validação corresponde a até 11 bytes; confirmar contra a base selecionada.
- Validar tamanhos PSK permitidos pelo protocolo. O codec do Tab5 aceita 0, 1, 16 ou 32 bytes; tamanho 0 pode significar herança e não simplesmente ausência de criptografia.
- Não expor controles MQTT de uplink/downlink no MVP; preservar compatibilidade do link sem habilitar MQTT ou rede.

## Digitar e revisar link

Fluxo: **Canais → Importar → Digitar link → Revisar → Adicionar ou Substituir → Confirmar → Persistir e aplicar → Resultado**.

O editor aceita link completo com capacidade proposta de 2.048 bytes, rascunho SD e cursor/rolagem. Uma opção adicional pode carregar `.url` do cartão. Ambas convergem para o mesmo parser offline; digitação deve continuar sendo suficiente.

Codec a portar do Tab5:

- Reconhece `https://meshtastic.org/e/#…` e `https://meshtastic.org/e/?add=true#…`.
- Decodifica Base64 URL-safe e ChannelSet nanopb; valida padding, bits residuais, tamanho, número de canais, UTF-8 e PSKs.
- Não acessa servidor, DNS ou internet. O payload vem no fragmento do link.
- Não tratar links abreviados/redirecionados como configuração disponível offline. Apresentar erro curto para formato não suportado.
- Eventual suporte a outras formas oficiais será adicionado somente com fixtures/especificação; nunca presumir que o parser atual aceita todas.

A tela de revisão pagina nome e papel dos canais, quantidade de slots necessários, mudança no primário e presença de LoRa. Não exibir PSKs completas em resumo. `add=true` sugere Adicionar, mas o usuário confirma o modo na UI.

## Adicionar

1. Ler snapshot consistente da configuração atual.
2. Normalizar PSKs efetivas da origem, resolvendo secundários sem chave contra o primário **do conjunto importado**.
3. Identificar duplicatas de configuração equivalente; mesmo nome com chave diferente não é duplicata.
4. Calcular slots livres secundários para todos os canais a adicionar, também considerando duplicatas dentro do conjunto importado.
5. Se faltar espaço, não modificar nada e mostrar quantos slots faltam.
6. Manter o primário e LoRa atuais. Avisar se os parâmetros do link indicarem que o canal adicionado pode não comunicar com a rede de origem sob o LoRa atual.
7. Persistir configuração completa numa geração nova e aplicar pelo backend local.

## Substituir

1. Mostrar canais que serão removidos/alterados e qual será o primário.
2. Construir conjunto completo: primeiro canal importado como primário, demais secundários; limpar/desativar slots excedentes sem deixar configurações residuais.
3. Revisar separadamente aplicação de LoRa presente no link. Preservar região, potência e restrições locais; validar parâmetros contra hardware e região atuais.
4. Não mudar configurações durante a prévia; abandonar revisão conserva estado original.
5. Commit de geração SD e aplicação no núcleo coordenados, com readback de configuração e eventual reinício controlado.
6. Revalidar conversa selecionada/rascunhos contra as novas identidades de canal; nunca enviar rascunho antigo a outro canal por reutilização do slot.

Sem SD gravável, criação/importação fica indisponível com motivo em tela. Falha de aplicação após persistência exige resultado parcial/recuperação explícita, conforme storage; não afirmar sucesso completo.

## Exportar

Gerar link de um canal ou conjunto usando o codec alinhado. Canal secundário exportado isoladamente deve carregar chave efetiva resolvida, para não herdar o primário diferente do receptor.

Mostrar URL com rolagem e salvar arquivo em `/CardputerADVMeshtastic/exports/`. Link e arquivo contêm chaves compartilháveis: exibir indicação curta antes de revelar/exportar. Não há câmera. QR para conjuntos longos fica fora do MVP devido ao tamanho da tela; exportação por texto/arquivo é suficiente.

## Critérios obrigatórios

- Criar canal, editar e reiniciar conserva nome/chave/slot.
- Digitar link real e importar sem internet funciona.
- Link inválido, UTF-8 inválido, PSK inválida, número excessivo ou falta de slots não altera configuração.
- Adicionar resolve herança contra a origem e preserva primário/LoRa atuais.
- Substituir mantém exatamente um primário, valida LoRa e conserva parâmetros locais definidos.
- Exportar e importar em implementação Meshtastic compatível mantém chaves e interoperabilidade.
