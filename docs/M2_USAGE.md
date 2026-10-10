# Operar o candidato M2

Versão `0.2.0-m2`, aplicação OTA para M5Launcher. O binário é maior que M1; comparar o tamanho no manifesto com o slot aceito pelo instalador. Versão/layout/limite instalado ainda não conhecido. Não instalar tabela de partições ou bootloader gerados no build.

## Primeiro boot

Usar Cap oficial com antena e microSD gravável. A tela mostra início do núcleo; a detecção GPS oficial pode levar alguns segundos. Após inicialização, deve aparecer `M2 pronto`, `SD OK`, ID do nó e `RF OK`. Sem Cap/cartão ou com arquivos incompletos, o núcleo fica parado e a tela informa o motivo; teclado/tela continuam disponíveis. Corrigir o cartão/acessório e reiniciar pelo procedimento já usado no launcher.

A configuração M2 ocupa um namespace novo e conserva os arquivos de diagnóstico M1. Não importa automaticamente configurações de outro aplicativo ou firmware.

## Teclado e configuração

- `Tab` / `Shift+Tab`: status, páginas de nós e ajuda provisória. Não são as quatro abas finais de M4.
- Digitar texto; `Fn+Enter` envia broadcast pelo canal primário padrão (índice 0 neste candidato). `Enter` simples executa somente comandos iniciados por `/`.
- Backspace remove; Fn+setas esquerda/direita move cursor; Fn+Esc limpa o campo. Na lista de nós, Fn+setas cima/baixo muda página.
- Comandos aceitos: `/region NOME`, `/preset NOME`, `/slot NUMERO`, `/hops NUMERO`, `/name CURTO Nome longo`.
- Nome curto: até quatro bytes; longo: até 39 bytes. Texto M2: até 200 bytes. Não há chat/histórico definitivo nem importação de canais nesta etapa.

Exemplo de sintaxe, **escolhendo a região correspondente ao local físico de uso**:

```text
/region BR_902
/preset LONG_FAST
/slot 0
/hops 3
/name ADV Cardputer ADV
```

Cada linha é digitada e confirmada por Enter. `BR_902` é um exemplo, não escolha automática. Para desativar TX, executar primeiro `/slot 0` e depois `/region UNSET`. O firmware rejeita regiões de 433 MHz, 2 m e 2,4 GHz para este produto. Aceita os identificadores sub-GHz compatíveis da revisão fixada; `BR_902`, `US`, `EU_868`, `ANZ` são exemplos.

`/slot 0` usa a seleção automática do núcleo; valores explícitos começam em 1 e precisam caber na região/preset. `/hops` aceita 0–7, com semântica/default do núcleo. Presets incluem `LONG_FAST`, `LONG_SLOW`, `MEDIUM_FAST`, `SHORT_FAST` e os demais nomes oficiais da revisão. Os parâmetros normalizados efetivamente aplicados aparecem no status. Ajustes de preset/slot/hops preservam os outros valores atuais.

Ao selecionar região pela primeira vez, o núcleo gera as chaves necessárias através do CryptoEngine oficial e salva a configuração no SD antes de habilitar o rádio. Não exibir/exportar chaves privadas nesta tela.

## O que observar

1. Confirme boot, orientação, teclado, `SD OK` e `RF OK`.
2. Sem região definida, uma tentativa de envio deve informar `Escolha regiao antes de TX`.
3. Configure nome, região e preset escolhidos. Confira `Configuracao salva/aplicada`; reinicie e confirme os valores e o ID do nó.
4. GPS: confira UTC, fix e satélites. Sem fix, a hora pode estar disponível. O clock continua baseado na referência GPS adquirida pelo núcleo.
5. Digite uma mensagem curta e use Fn+Enter. `Fila local` não comprova transmissão. `TX radio` vem de TX_DONE e não comprova entrega a outro nó. Contador TX também inclui anúncios automáticos do núcleo, não apenas texto.
6. Lista de nós: local, cache ou `RX boot`. RX zerado sem outro emissor não comprova falha. Para interoperabilidade futura, outro nó precisa usar região/preset/slot/canais compatíveis; então confirmar texto, posição, métricas e feedback.

Com falha do SD, não interpretar conteúdo em RAM como salvo. Evitar retirar o cartão com a aplicação ativa. O M3 será responsável pela recuperação de escrita interrompida e pelo histórico completo.
