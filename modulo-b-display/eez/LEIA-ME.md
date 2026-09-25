# Interface do Módulo B no EEZ Studio

As telas do display são desenhadas no **EEZ Studio** e exportadas como código
C para `modulo-b-display/src/ui/`. O firmware não mexe nesses arquivos. Ele
conversa com a UI por dois canais:

- **Native global variables**: a UI mostra, e `src/ui_ponte.c` mantém atualizado.
- **Actions**: os botões chamam, e `src/ui_ponte.c` implementa.

Enquanto o projeto EEZ não existir, `src/ui/` tem um **placeholder** feito à
mão no mesmo formato, que já funciona para testar o enlace. O primeiro Build
do EEZ sobrescreve esse placeholder.

## 1. Criar o projeto

1. EEZ Studio → *New Project* → tipo **LVGL** (sem Flow).
2. **LVGL version: 9.x.** O projeto de teste antigo era 8.4, mas este firmware usa LVGL 9.
3. Display **800 × 480**.
4. Salve o projeto **nesta pasta**: `modulo-b-display/eez/telemetria.eez-project`.
5. Em *Settings → Build*:
   - **Destination folder:** `../src/ui`
   - **LVGL include:** `lvgl.h`. O padrão `lvgl/lvgl.h` **não compila** no
     ESP-IDF, porque o componente fica em `managed_components/lvgl__lvgl`.

## 2. Telas (nomes e ordem obrigatórios)

A primeira da lista é a que abre no boot. A ponte navega por `SCREEN_ID_<NOME>`.

| # | Nome no EEZ | Conteúdo |
|---|---|---|
| 1 | `principal` | RPM, velocidade, consumo, etanol, temperatura, litros, tensão |
| 2 | `alarme` | Limiar, histerese e janela com botões −/+, botão Salvar |
| 3 | `dtc` | Lista vazia por enquanto (depende do Modo 03, fase 2) |
| 4 | `enlace` | RSSI, recebidos, perdidos, entrega, taxa efetiva |
| 5 | `config` | Taxa do CAN do Módulo A, botões 250k / 500k |

## 3. Variáveis (Project → Variables → Global, marcar **Native**)

Nome exatamente como abaixo, em snake_case. O EEZ gera `get_var_<nome>()`, e
um nome diferente dá erro de link (é proposital, para acusar a divergência).

Para ligar um Label: *Text* → escolha a variável. Para ligar a visibilidade:
*Flags → Hidden* → a variável booleana.

| Variável | Tipo | Tela | Exemplo |
|---|---|---|---|
| `rpm_txt` | string | principal | `2350` |
| `rpm` | integer | principal | 0..7000, para Bar/Arc/Scale |
| `vel_txt` | string | principal | `87` |
| `consumo_txt` | string | principal | `12.4` |
| `consumo_unid_txt` | string | principal | `km/L` andando, `L/h` parado |
| `etanol_txt` | string | principal | `27 %` |
| `temp_txt` | string | principal | `90.5 °C` |
| `litros_txt` | string | principal | `29.7 L` |
| `tensao_txt` | string | principal | `14.10 V` |
| `alarme_oculto` | boolean | principal | ligar em *Hidden* da faixa de alarme |
| `aviso_enlace_oculto` | boolean | principal | ligar em *Hidden* do aviso "sem sinal" |
| `limiar_txt` | string | alarme | `105.0 °C` |
| `histerese_txt` | string | alarme | `3.0 °C` |
| `janela_txt` | string | alarme | `8 amostras` |
| `media_txt` | string | alarme | média móvel atual |
| `estado_alarme_txt` | string | alarme | `NORMAL` / `ALARME` / `SEM DADOS` |
| `salvo_oculto` | boolean | alarme | *Hidden* do "salvo na NVS" |
| `estado_enlace_txt` | string | enlace | `CONECTADO` / `SEM SINAL` |
| `rssi_txt` | string | enlace | `-52 dBm` |
| `recebidos_txt` | string | enlace | |
| `perdidos_txt` | string | enlace | |
| `entrega_txt` | string | enlace | `99 %` |
| `taxa_txt` | string | enlace | `10.0 pct/s` |
| `can_taxa_txt` | string | config | `250 kbit/s` (o que o Módulo A está usando) |
| `can_firmware_txt` | string | config | `bancada` / `carro` |
| `can_pedido_txt` | string | config | andamento do último pedido de troca |

Não precisa usar todas. Uma variável que o projeto não declara só fica sem uso.

## 4. Actions (Project → Actions)

Crie com estes nomes e ligue cada uma ao evento **CLICKED** do botão.

| Action | Onde |
|---|---|
| `ir_principal`, `ir_alarme`, `ir_dtc`, `ir_enlace`, `ir_config` | navegação (em todas as telas) |
| `limiar_menos`, `limiar_mais` | alarme |
| `histerese_menos`, `histerese_mais` | alarme |
| `janela_menos`, `janela_mais` | alarme |
| `salvar_alarme` | alarme |
| `can_250k`, `can_500k` | config |

## 5. Fontes

O LVGL do firmware tem as Montserrat 12, 14, 16, 18, 20, 24, 28, 32, 36, 40 e 48.
Se usar outro tamanho no EEZ, habilite `CONFIG_LV_FONT_MONTSERRAT_<n>` em
`sdkconfig.defaults` e apague o `sdkconfig.modulo_b` para ele ser regenerado.

Todas incluem o `°`. Acentos (ã, ç, é) **não** vêm nas fontes embutidas. Para
usá-los, adicione uma fonte própria no EEZ com a faixa de caracteres latinos.

## 6. Fluxo de trabalho

1. Desenhe no EEZ → **Build** (F7), que escreve em `src/ui/`.
2. No VS Code, compile o `modulo-b-display` normalmente.
3. Se der erro de link `undefined reference to get_var_x`, a variável `x` foi
   declarada no EEZ e não existe na ponte. Confira o nome na tabela acima.

Se as cores saírem com vermelho e azul trocados, mude o *Color format* nas
configurações do projeto EEZ.
