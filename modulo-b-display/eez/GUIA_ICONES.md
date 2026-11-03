# Ícones no EEZ Studio — passo a passo

Os ícones são PNG brancos de 48 e 32 px, importados como bitmap **A8** (só
transparência). A cor é dada pelo firmware (`src/ui_icones.c`): branco no
normal, verde para OK, âmbar para atenção e vermelho para alarme. É o mesmo
formato do `btn_back` que já funciona na placa.

## 0. Antes de abrir o EEZ

Com o **EEZ fechado**, na raiz do repositório:

```
python ferramentas/icones/injetar_bitmaps_eez.py modulo-b-display/eez/projeto_display_carro.eez-project
```

Isso coloca 28 bitmaps `ic_*` no projeto, já com **Always build** marcado e
faz uma cópia de segurança `.antes-icones.bak`. Rodar de novo só atualiza.

Abra o projeto e confira a aba **Bitmaps**: devem aparecer `ic_rpm`,
`ic_velocidade`, ..., `ic_sinal_3_32`. Clique em um deles e confira que o
formato de cor é o mesmo do `btn_back` (A8) e que **Always build** está
marcado. Sem o Always build, o EEZ só gera os bitmaps que estão colocados em
alguma tela, e o firmware não compila (`undefined reference to img_ic_...`).

## 1. Painel principal — obrigatório

Use o widget **Image** (não Imgbutton). Em cada um: preencha **Name**
exatamente como na tabela e escolha qualquer bitmap em **Image** (o firmware
troca em tempo de execução; o que você escolhe aqui só aparece no preview).

| Name | Bitmap no EEZ | Left | Top | W × H | Ao lado de |
|---|---|---|---|---|---|
| `icon_v1` | `ic_rpm` | 45 | 157 | 48 × 48 | Value_1 |
| `icon_v2` | `ic_velocidade` | 45 | 238 | 48 × 48 | Value_2 |
| `icon_v3` | `ic_temp_motor` | 45 | 319 | 48 × 48 | Value_3 |
| `icon_v4` | `ic_consumo` | 389 | 157 | 48 × 48 | Value_4 |
| `icon_v5` | `ic_etanol` | 389 | 238 | 48 × 48 | Value_5 |
| `icon_v6` | `ic_tanque` | 389 | 319 | 48 × 48 | Value_6 |
| `icon_status` | `ic_can_ok_32` | 28 | 420 | 32 × 32 | linha de status |
| `icon_sinal` | `ic_sinal_3_32` | 740 | 36 | 32 × 32 | canto superior direito |

Na linha de status: apague o label fixo **"Status:"** e mova o label de
`msg_status` para **Left 70** (mantendo o Top 422). O ícone substitui a
palavra.

Os nomes viram `objects.icon_v1` etc. no `screens.h`. Se faltar algum, a
compilação acusa `'objects_t' has no member named 'icon_...'`, que é o
aviso de que o nome no EEZ não bate.

## 2. Menu principal — recomendado

Dentro de cada botão, junto do label, arraste um **Image** 48 × 48 (ele tem
que ficar como *filho* do botão na árvore de widgets):

| Botão | Bitmap |
|---|---|
| `btn_settings` | `ic_configuracoes` |
| `btn_panel` | `ic_painel` |
| `btn_layout` | `ic_layout` |
| `btn_alarms` | `ic_alarmes` |

Posição: à esquerda do botão, centralizado na vertical (o botão tem 69 px de
altura). O botão tem padding interno, então confira no preview e ajuste o
Left/Top à mão. Esses não precisam de Name: o firmware pinta de branco
toda imagem que encontra nas telas.

## 3. Alarmes e Config — opcional

- Tela **Painel Alarms**: `ic_superaquecimento`, `ic_motor_frio` e `ic_eco`
  ao lado de cada label. Vai precisar empurrar labels e sliders uns 50 px
  para a direita para caber.
- Tela **Painel Config**: `ic_tentar_denovo_32` dentro do botão Retry.

## 4. Build

1. **Build (F7)** no EEZ. Ele cria um `ui_image_ic_*.c` por bitmap em `src/ui/`.
2. No VS Code: **PlatformIO → Clean** do `modulo-b-display` (arquivos `.c`
   novos em `src/ui/` só entram depois de um Clean, ver `src/CMakeLists.txt`),
   depois Build e Upload.

## O que você deve ver na placa

- Cada campo com o ícone da grandeza que está escolhida no Layout. Trocar a
  grandeza no Layout troca o ícone.
- Linha de status: ícone vermelho de sem sinal enquanto o Módulo A não
  conecta, âmbar de CAN quando não há dados válidos, verde de OK em
  operação. No simulador, o comando `q` (108 °C) deixa o ícone de
  superaquecimento vermelho e o ícone do campo de temperatura do motor
  também vermelho.
- Barras de sinal: 3 barras verdes acima de −60 dBm, 2 até −70, 1 âmbar até
  −80, vazia vermelha abaixo disso. Sem enlace aparece o ícone cortado.

Se um ícone aparecer **preto** (invisível no fundo preto, mas o lugar dele
existe), é recolor faltando: confira se ele é um widget Image (Imgbutton é
tratado à parte, por `colorir_icone()` na ponte).
