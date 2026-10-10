# M7 — Primeira entrega: mensagens diretas

1. Na aba NÓS, escolha um nó com Fn+setas e abra o detalhe com Enter.
2. Pressione Enter novamente para abrir a DM. Se aparecer “aguarde NodeInfo PKI”, o ADV ainda não conhece a chave pública desse nó. Aguarde o NodeInfo recebido pela rede; não há envio legado automático.
3. A linha superior identifica `DM !xxxxxxxx PKI`. Digite e use Fn+Enter para enviar. O envio só segue para o rádio depois de gravado no SD.
4. Fn+Esc volta do editor para a lista. Outro Fn+Esc abre o seletor de canais; Enter abre o canal escolhido. Para reabrir uma DM, use o detalhe do mesmo nó.
5. No histórico, Enter abre a mensagem. A entrega distingue fila local, TX pelo rádio, ACK, falha/NAK e timeout. Fn+Esc volta. O núcleo controla as tentativas; o app não dispara retransmissão própria.

Cada DM tem destinatário, rascunho/cursor e histórico próprios. Reiniciar deve restaurar a conversa e o rascunho abertos. Uma DM recebida em outra conversa é gravada no SD e gera aviso, sem mudar foco ou interromper a edição; abra o detalhe do remetente para consultar. Nesta primeira entrega, o seletor continua listando canais: não há lista independente de DMs nem contador persistente de não lidas para cada nó.

O destinatário precisa estar no banco de nós e ambos os dispositivos precisam ter chaves PKI. A modalidade de radioamador/licenciado não permite esta função. A identidade da conversa inclui os IDs e as chaves públicas dos dois nós; uma troca de chave exige reabrir a DM e não reutiliza o rascunho do contexto anterior. O arquivo antigo fica preservado no cartão. Mensagens privadas legadas sem PKI não são incorporadas a esta conversa protegida.

## Conferência no ADV

- Com um nó compatível, testar envio e recebimento nos dois sentidos e verificar ACK abrindo novamente o detalhe da mensagem.
- Deixar um rascunho privado, reiniciar e conferir destinatário, texto e cursor; voltar ao canal e conferir seu rascunho independente.
- Receber uma DM enquanto escreve no canal e conferir que o texto/foco permanecem, depois abrir a DM pelo remetente.
- Com destinatário fora do alcance, aguardar o resultado do núcleo e conferir timeout; TX no rádio sozinho não confirma entrega.
- Conferir canais, importação/exportação, GPS e menu de 5 segundos como regressão.

Distribuição somente como aplicação OTA pelo M5Launcher. O agente não grava firmware nem altera partições. Busca/favoritos, distância, traceroute e backup serão entregas seguintes de M7.
