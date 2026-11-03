# Fontes do cockpit (Orbitron)

Fonte **Orbitron** (The League of Moveable Type), licença **SIL Open Font
License 1.1**: uso e distribuição livres, inclusive embutida em firmware.

Convertidas com `lv_font_conv` 1.5, 4 bpp, só com os caracteres usados, para
ocupar pouca flash (~66 kB no total):

| Arquivo | Tamanho | Caracteres | Uso |
|---|---|---|---|
| `ui_font_orb_80.c` | 80 px | 0–9 . , - espaço | RPM |
| `ui_font_orb_64.c` | 64 px | idem | velocidade |
| `ui_font_orb_56.c` | 56 px | idem | temperatura e tensão |
| `ui_font_orb_22.c` | 22 px | ASCII + ° + Latin-1 | unidades |
| `ui_font_orb_16.c` | 16 px | ASCII + ° + Latin-1 | rótulos e status |

Para gerar outro tamanho (Node.js):

```
npm i @fontsource/orbitron lv_font_conv
npx lv_font_conv --font node_modules/@fontsource/orbitron/files/orbitron-latin-600-normal.woff \
  -r 0x30-0x39,0x2D,0x2E,0x2C,0x20 --size 72 --bpp 4 --format lvgl --no-compress \
  --lv-include lvgl.h --lv-font-name ui_font_orb_72 -o ui_font_orb_72.c
```

Os números usam o peso 600; os textos, o 500.
