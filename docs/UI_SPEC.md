# Interface 240×135 e teclado

Implementação atual: [M6_STATUS.md](M6_STATUS.md) e [M6_USAGE.md](M6_USAGE.md). Canais editáveis e links estão em M6; DM permanece em M7.

## Organização

Quatro abas fixas: **CHAT / NÓS / CANAIS / AJUSTES**. GPS e diagnósticos ficam em subtelas de Ajustes; detalhes e DM partem de Nós. Fontes pequenas legíveis, seleção contrastante, poucas cores e ícones mínimos. Exibir um campo editável por vez.

Dimensões implementadas em M4, conferidas na renderização nativa:

- Barra superior: 16 px; rádio, hora curta, bateria e estado SD. `--:--` para hora desconhecida e indicador de SD sem persistência.
- Menu inferior visível: 18 px; quatro áreas de 60 px, aba ativa destacada.
- Conteúdo disponível: 101 px com menu; 119 px quando oculto.
- Menus longos e detalhes: linhas de 16 px, fonte nominal 12, com rolagem.

Exemplo esquemático de Chat (proporções ilustrativas):

```text
┌──────────────────────────────────────┐
│ RF OK       14:32       SD OK    72% │
│ #LongFast                     2 novas│
│ ANA 14:30: Cheguei ao ponto.          │
│ EU  14:31: Confirmado.                │
│ BOB 14:32: [texto; Enter abre]        │
│ > rascunho…                 32/limite│
│ CHAT       NÓS       CANAIS  AJUSTES │
└──────────────────────────────────────┘
```

Mensagens usam linhas compactas, não balões grandes. Detalhe mostra o conteúdo inteiro. Uma tela dedicada de composição pode ocupar a área central e manter o destino sempre visível; não há teclado virtual.

## Regra exata do menu

1. Na inicialização o menu aparece; registrar `last_menu_action` no contador monotônico.
2. Abrir o menu registra um novo prazo de 5.000 ms, mesmo sem trocar aba.
3. Mudar efetivamente para outra aba mantém menu visível e reinicia esse prazo.
4. Selecionar a aba que já está ativa não reinicia o prazo.
5. Aos 5.000 ms ou depois, esconder o menu; continuar na mesma aba e liberar a área inferior para conteúdo.
6. Digitar, mover cursor, rolar, receber mensagens, mudar canal da conversa, mostrar clock ou atualizar bateria **não** reinicia o prazo.
7. Ocultar menu não apaga rascunho, não altera aba, seleção ou leitura. O foco volta ao conteúdo se estava na barra.
8. Pode fechar antes por ação explícita. Subtelas e diálogos continuam funcionais quando a barra some; se o diálogo impedir troca de aba, a barra ainda expira normalmente.

Timeout de menu é independente de sono/apagamento do display. Não incluir ajuste de brilho ou menu de timeout de tela no MVP.

## Eventos de teclado propostos

Separar eventos lógicos de combinações físicas. As combinações abaixo serão verificadas no TCA8418 em M1; não exigem botão G0.

| Entrada candidata | Ação |
|---|---|
| Tab com menu oculto | Exibir menu e manter a aba atual |
| Tab com menu visível | Próxima aba; troca imediata |
| Shift+Tab com menu visível | Aba anterior; troca imediata |
| Setas lógicas | Mover seleção/rolagem; no editor, cursor |
| Enter | Abrir seleção ou confirmar campo; no composer, usar função definida para edição |
| Fn+Enter | Enviar texto/revisar formulário somente na tela apropriada |
| Escape lógico / combinação disponível | Voltar de subtela; na raiz fechar o menu |
| Backspace | Apagar no editor; não descartar texto ao voltar |

Mapear as setas e Escape às teclas/modificadores reais após validar leitura. Se `Fn+Enter` não estiver disponível, usar ação Enviar selecionável e confirmação por Enter, mantendo fluxo completo pelo teclado. Não permitir que a primeira tecla usada para abrir o menu envie mensagem.

## Telas

**Chat:** conversas por canal, não lidas, histórico paginado, composer e detalhe de mensagem. A conversa ativa define o destino; mudar aba preserva rascunho. Ao trocar configuração de canais, verificar identidade da conversa antes de permitir envio de rascunho antigo.

**Nós:** lista com nome curto/ID, sinal e idade; detalhe em páginas de identidade, rádio, GPS e métricas. Ausência de dado mostra `--`. Entrada DM fica disponível no marco M7.

**Canais:** lista de slots ativos/livres; criar, editar, importar link, exportar e desativar. Assistentes de um campo por tela. Nunca apresentar formulário com oito canais simultaneamente.

**Ajustes:** identidade, região/preset, offset UTC, GPS, notificações simples, cartão/backup, versão/diagnóstico e retorno ao instalador se houver mecanismo validado. Não apresentar seletores de transporte, Wi-Fi, brilho ou relógio manual.

## Editor de URL

- Link completo digitável, com `: / ? = # - _`, maiúsculas/minúsculas e símbolos de Base64, conforme teclado real.
- Rolagem horizontal/cursor, navegação por trecho quando possível, contador de bytes e erro de capacidade explícito.
- Buffer proposto 2.048 bytes incluindo terminador; validar contra o ChannelSet máximo da versão fixada.
- Rascunho salvo no SD e opção de limpar confirmada na própria tela.
- Decodificar só ao solicitar Revisar; não modificar configuração durante digitação.
- Importação não faz HTTP. O conteúdo após `#` contém o conjunto codificado; internet não é necessária.

## Notificações

Receber mensagem atualiza indicador da conversa e pode emitir beep curto. Não abre popup longo, não muda aba, não revela menu nem move a página que o usuário está lendo. Mensagem pode atualizar o fim do histórico quando o usuário já está acompanhando o fim.
