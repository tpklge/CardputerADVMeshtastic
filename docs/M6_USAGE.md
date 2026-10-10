# Uso M6 — canais standalone

Firmware OTA: `newversion/m6/cardputer-adv-0.6.0-m6-candidate-ota.bin`, pelo M5Launcher já utilizado. Manter o microSD com os dados anteriores. A fonte e a navegação de M4/M5 permanecem.

## Criar um segundo canal

1. Tab até **Canais**. Fn+cima/baixo escolhe um slot **Livre**, por exemplo 1.
2. Enter abre ações; Enter em **Editar / criar**.
3. Digitar nome de 1 a 11 bytes; Enter.
4. O firmware já gerou uma chave AES256 aleatória. Enter mantém essa chave; Enter na revisão confirma a gravação.
5. Aguardar **Canais salvos/aplicados**. No **Chat**, Fn+Esc na raiz abre conversas; escolher o novo canal e Enter.

Para interoperar nesse canal, o outro nó precisa do mesmo nome/chave e LoRa compatível; compartilhar pelo link exportado. Criar o canal não faz outros nós entrarem nele automaticamente.

## Editar, chave e desativar

Enter sobre um slot abre **Editar/criar**, **Gerar chave AES256**, **Desativar secundário**, **Importar link**, **Exportar canal** e **Exportar conjunto**. Fn+setas escolhe; Enter abre. Fn+Esc cancela e volta à lista sem aplicar.

A chave pode ser digitada em Hex ou Base64 padrão; aceita 0, 1, 16 ou 32 bytes. Campo vazio conserva a semântica oficial: secundário herda o primário, primário fica sem criptografia. `00` explicita canal aberto; `01` representa a chave pública padrão. AES128 usa 32 caracteres hex e AES256, 64. A revisão informa tamanho sem revelar a chave. Gerar uma chave exige confirmação antes de aplicar.

Somente secundários podem ser desativados pelo editor. Trocar o primário exige importar/substituir. Renomear um primário pode alterar a frequência automática; a revisão alerta sobre isso. Radio/SD ocupados impedem aplicar: aguardar e reabrir as ações para tentar novamente.

## Importar link digitado

**Canais → Enter → Importar link**. Digitar `https://meshtastic.org/e/#...` ou `https://meshtastic.org/e/?add=true#...`. O parser funciona offline; não acessa servidor ou redirecionamentos.

Editor de até **2.047 bytes**, com Backspace e Fn+esquerda/direita para cursor/rolagem. Rascunho/cursor do link são conservados no SD após pausa de aproximadamente 1 segundo, inclusive após reboot. Enter decodifica e abre escolha do modo:

- Fn+cima/baixo alterna **Adicionar / Substituir**.
- Em Substituir, Fn+direita alterna aplicação do LoRa do link quando presente.
- Enter abre revisão; Fn+cima/baixo percorre os oito slots propostos.
- Enter abre confirmação final; outro Enter aplica. Fn+Esc cancela sem alterar canais.

Adicionar resolve chaves herdadas contra o primário da origem, remove duplicatas lógicas e exige espaço para o conjunto inteiro. Mantém primário e LoRa locais; LoRa diferente no link pode impedir comunicação com a origem. Substituir faz o primeiro canal virar primário e desativa excedentes. Aplicação opcional de LoRa aceita presets da base; configuração customizada, override de frequência ou offset RF não são aplicados. Região, potência, hops e demais restrições locais são preservados. Mudança do primário pode alterar a frequência automática mesmo conservando campos de LoRa.

Links/chaves inválidos e falta de espaço não alteram configuração. Não há importação de arquivo `.url` neste marco: digitação é o fluxo implementado.

## Exportar

**Exportar canal** compartilha o slot selecionado com chave efetiva, resolvendo herança. **Exportar conjunto** inclui primário e secundários ativos. A confirmação informa que o link contém chaves. Enter revela o texto e solicita arquivo em `/CardputerADVMeshtastic/exports/channels-<geracao>.url`.

Fn+setas percorre a URL. A tela indica fila de gravação e depois **Link salvo em exports/*.url**, após readback do arquivo. Falha de SD aparece na barra/mensagem. Exportação também passa a ser o rascunho de link, reutilizável na importação.

## Conferência no ADV

Criar um secundário, selecioná-lo no Chat, escrever um rascunho diferente do LongFast e reiniciar para conferir nome/chave/conversa/rascunhos. Exportar e conferir o arquivo no cartão; importar link digitado e revisar adicionar/substituir. Para validar RF, usar outro nó com o link exportado e LoRa compatível. Dados anteriores devem permanecer disponíveis. Nenhum firmware foi enviado pelo agente.
