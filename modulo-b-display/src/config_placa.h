/**
 * config_placa.h — TODA a pinagem e temporização da placa Sunton num único
 * lugar. Nenhum pino aparece solto em outro arquivo: para portar a outra
 * placa (ou corrigir com os valores do demo do fabricante), troca-se SÓ este
 * header.
 *
 * Valores conferidos no esquemático da ESP32-8048S043 e validados no
 * hardware pelo projeto de teste "iniciando com LGVL" (Arduino_GFX + LVGL 8),
 * onde painel e touch funcionaram com exatamente estes números.
 */
#pragma once

/* ---------- Painel RGB 800x480 ---------- */
#define PLACA_LCD_H_RES            800
#define PLACA_LCD_V_RES            480

/* Clock de pixel e porches — validados no teste (14 MHz estável) */
#define PLACA_LCD_PCLK_HZ          (14 * 1000 * 1000)
#define PLACA_LCD_HSYNC_PULSO      4
#define PLACA_LCD_HSYNC_BACK_PORCH 8
#define PLACA_LCD_HSYNC_FRONT_PORCH 8
#define PLACA_LCD_VSYNC_PULSO      4
#define PLACA_LCD_VSYNC_BACK_PORCH 8
#define PLACA_LCD_VSYNC_FRONT_PORCH 8
/* Polaridades: o teste usou hsync/vsync_polarity = 0 no Arduino_GFX, que ele
 * traduz para hsync_idle_low/vsync_idle_low = 1 no esp_lcd; PCLK amostrado
 * na borda de descida. */
#define PLACA_LCD_HSYNC_IDLE_LOW   1
#define PLACA_LCD_VSYNC_IDLE_LOW   1
#define PLACA_LCD_PCLK_ATIVO_NEG   1

/* Sinais de controle — conferidos no esquemático */
#define PLACA_LCD_GPIO_DE          40
#define PLACA_LCD_GPIO_VSYNC       41
#define PLACA_LCD_GPIO_HSYNC       39
#define PLACA_LCD_GPIO_PCLK        42

/* Barramento de dados RGB565: D0..D15 = B0..B4, G0..G5, R0..R4
 * (conferidos no esquemático) */
#define PLACA_LCD_GPIO_D0          8   /* B0 */
#define PLACA_LCD_GPIO_D1          3   /* B1 */
#define PLACA_LCD_GPIO_D2          46  /* B2 */
#define PLACA_LCD_GPIO_D3          9   /* B3 */
#define PLACA_LCD_GPIO_D4          1   /* B4 */
#define PLACA_LCD_GPIO_D5          5   /* G0 */
#define PLACA_LCD_GPIO_D6          6   /* G1 */
#define PLACA_LCD_GPIO_D7          7   /* G2 */
#define PLACA_LCD_GPIO_D8          15  /* G3 */
#define PLACA_LCD_GPIO_D9          16  /* G4 */
#define PLACA_LCD_GPIO_D10         4   /* G5 */
#define PLACA_LCD_GPIO_D11         45  /* R0 */
#define PLACA_LCD_GPIO_D12         48  /* R1 */
#define PLACA_LCD_GPIO_D13         47  /* R2 */
#define PLACA_LCD_GPIO_D14         21  /* R3 */
#define PLACA_LCD_GPIO_D15         14  /* R4 */

/* Backlight — conferido no teste */
#define PLACA_LCD_GPIO_BACKLIGHT   2
#define PLACA_LCD_BACKLIGHT_LIGADO 1   /* nível lógico que acende */

/* Bounce buffer em SRAM interna, em linhas do painel.
 * 10 linhas × 800 px × 2 B = 16 KB — ver justificativa em display_init.c */
#define PLACA_LCD_BOUNCE_LINHAS    10

/* ---------- Touch GT911 (I2C) ---------- */
#define PLACA_TOUCH_GPIO_SDA       19
#define PLACA_TOUCH_GPIO_SCL       20
/* INT do GT911 NÃO vai a GPIO nenhuma nesta placa (conferido no
 * esquemático). Consequência: não dá para escolher o endereço I2C pelo nível
 * do INT no reset — ver display_init.c. */
#define PLACA_TOUCH_GPIO_INT       (-1)
#define PLACA_TOUCH_GPIO_RST       38
#define PLACA_TOUCH_I2C_HZ         400000

/* Faixa BRUTA que o GT911 desta placa entrega, medida no teste tocando os 4
 * cantos: 0..480 x 0..272. O chip está com a configuração de fábrica de um
 * painel 480x272 e não aceita reconfiguração de resolução com segurança,
 * então a escala para 800x480 é feita no firmware (display_init.c). */
#define PLACA_TOUCH_BRUTO_X_MAX    480
#define PLACA_TOUCH_BRUTO_Y_MAX    272
