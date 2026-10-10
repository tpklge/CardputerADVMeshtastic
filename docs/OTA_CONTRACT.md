# Contrato de entrega por OTA

## Regra do projeto

O firmware será entregue ao ADV **via OTA pelo fluxo de instalação já existente**. A aplicação não terá Wi-Fi nem atualizador de rede próprio; o instalador existente pode usar sua conectividade fora da execução deste app.

O usuário confirmou **M5Launcher**, do mesmo desenvolvedor do launcher Tab5. Versão/layout/tamanho do slot instalado no ADV ainda não foram informados. Não copiar contrato do Tab5 ESP32-P4 para o ADV ESP32-S3.

Há build e candidato de diagnóstico M1, exclusivamente de aplicação. Não houve gravação, envio ou mudança no aparelho. `config/ota_contract.json` conserva as pendências e o empacotador recusa pacote de release quando o contrato instalado não está verificado.

## O que verificar antes do build definitivo

| Dado | Estado atual |
|---|---|
| Instalador e versão em uso no ADV | M5Launcher confirmado; versão ainda desconhecida |
| Target | ESP32-S3 / Cardputer ADV |
| Tabela de partições instalada | A verificar, preservando a instalação existente |
| Limite máximo do app aceito | A verificar; 8 MB de flash não é o tamanho do slot |
| Formato exato de aplicação aceito | A verificar no instalador |
| Retorno ao instalador | A verificar; não presumir deep sleep do Tab5 |
| Recuperação de aplicação que não inicia | A verificar no fluxo existente |
| Preservação de NVS/dados técnicos | Não apagar na entrega do app |

O upstream possui sua tabela padrão e protocolo de upload próprios. Não adotá-los automaticamente. Um firmware instalado por launcher pode usar um layout diferente de OTA convencional com `ota_0`/`ota_1`; não criar `partitions.csv` ativa até conhecer o contrato.

## Pacote futuro

```text
newversion/
  cardputer-adv-meshtastic-<versao>-ota.bin
  SHA256SUMS.txt
  manifest.json
  RELEASE_NOTES.md
```

Manifesto: versão do produto, alvo, upstream/revisão própria, data UTC do build, toolchain, tamanho/checksum, schema SD, requisitos do instalador e limite de app utilizado na validação.

O `.bin` deve ser somente imagem de aplicação compatível. Não entregar imagem merged/full-flash, bootloader ou tabela de partições como se fossem aplicação OTA. Usar inspeção offline do binário para conferir arquitetura/formato/tamanho.

## Ferramenta futura

Criar ferramenta de **build e empacotamento**, sem envio:

1. Verificar revisão/dependências e configuração de alvo/partições compatíveis.
2. Compilar sem `upload`, monitor serial ou conexão ao aparelho.
3. Verificar imagem de aplicação e comparar com o limite real do instalador.
4. Copiar aplicação para `newversion/`, produzir checksum e manifesto.
5. Informar pacote produzido e verificações realizadas.

Não aceitar argumento de porta nem opção de flash. O `build_ota.sh` do Tab5 tem gravação opcional ao receber porta: esse comportamento não deve ser transportado.

## Dados após atualização

Preservar `/CardputerADVMeshtastic/` no microSD. A primeira execução deve validar schema e migrar de forma não destrutiva, conservando geração anterior. Downgrade para firmware com schema desconhecido entra em recuperação, sem substituir arquivos por defaults.

Preparar o pacote não implica enviá-lo nesta etapa. Validação OTA no hardware será um trabalho posterior dentro do escopo solicitado para implementação/instalação; nenhum teste exige gravação por USB.

## Build M1 existente

`.venv/bin/python tools/build_ota.py --diagnostic` compila e prepara `newversion/m1/`. A tabela do board `default_8MB.csv` serve somente à linkedição offline. O pacote inclui aplicação, SHA-256 e manifesto, com `installation_compatibility_verified: false` enquanto versão/slot instalados não forem comprovados. `tools/pio_build.py` permite apenas alvos offline.
