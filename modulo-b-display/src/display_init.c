/**
 * display_init.c — Painel RGB 800x480 + GT911 + esp_lvgl_port.
 *
 * Decisões que serão defendidas em banca:
 *
 *  1. DUPLO FRAMEBUFFER EM PSRAM (num_fbs = 2) + modo full_refresh/avoid
 *     tearing do esp_lvgl_port: o LVGL desenha sempre no framebuffer que NÃO
 *     está sendo varrido e a troca acontece no vsync. É isso que elimina o
 *     tearing — um framebuffer único obrigaria a escrever no mesmo buffer que
 *     o controlador RGB está lendo, e o rasgo ficaria visível em qualquer
 *     animação. Não substituir por framebuffer único.
 *
 *  2. BOUNCE BUFFER EM SRAM INTERNA: o controlador RGB varre o painel sem
 *     parar; se ele lesse a PSRAM diretamente, qualquer pico de tráfego no
 *     barramento octal (Wi-Fi, cache miss, log em flash) atrasaria o fetch e
 *     deslocaria a imagem. Com o bounce buffer, o DMA copia blocos de linhas
 *     da PSRAM para a SRAM e o controlador consome da SRAM, tolerando o
 *     jitter. Par obrigatório com CONFIG_LCD_RGB_ISR_IRAM_SAFE e
 *     CONFIG_SPIRAM_FETCH_INSTRUCTIONS/RODATA (sdkconfig.defaults).
 *
 *  3. TASK DO LVGL FIXADA NO CORE 1: o stack Wi-Fi/ESP-NOW roda no core 0.
 *     Renderização (que pode levar dezenas de ms num frame cheio) nunca
 *     compete com a recepção de pacotes.
 *
 *  4. RESET DO GT911: o endereço I2C do GT911 (0x5D ou 0x14) é AMOSTRADO do
 *     nível do pino INT durante a borda de subida do reset. Nesta placa o INT
 *     NÃO está ligado a GPIO nenhuma (conferido no esquemático), então não dá
 *     para impor o endereço. A sequência aqui é: reset manual pelo RST
 *     (baixo -> alto), espera o chip acordar, e SONDA os dois endereços no
 *     barramento. O driver é criado já com o endereço encontrado e com
 *     rst_gpio_num = -1, para não resetar de novo (um segundo reset poderia
 *     sortear o outro endereço e o driver ficaria falando no vazio).
 *
 *  5. ESCALA DO TOUCH: o GT911 desta placa entrega 0..480 x 0..272 (config de
 *     fábrica de painel 480x272, medida no projeto de teste). A conversão
 *     para 800x480 é feita no callback process_coordinates, antes de o LVGL
 *     ver o ponto.
 */
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lvgl_port.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config_placa.h"
#include "display_init.h"

static const char *TAG = "display";

/* Task do LVGL: core, prioridade e stack. Stack folgado porque telas com
 * muitos widgets estouram os 4 KB padrão durante o layout. */
#define LVGL_TASK_CORE       1
#define LVGL_TASK_PRIORIDADE 4
#define LVGL_TASK_STACK      (8 * 1024)

/* Temporização do reset do GT911 (datasheet: RST baixo >= 100 us; após
 * subir, >= 50 ms antes de conversar no I2C) — com folga */
#define GT911_RST_BAIXO_MS       10
#define GT911_ACORDAR_MS         100
#define GT911_TIMEOUT_SONDA_MS   50
#define GT911_ENDERECO_PRIMARIO  0x5D
#define GT911_ENDERECO_ALTERNATIVO 0x14

static esp_err_t iniciar_backlight(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PLACA_LCD_GPIO_BACKLIGHT,
        .mode = GPIO_MODE_OUTPUT,
    };
    esp_err_t r = gpio_config(&cfg);
    if (r == ESP_OK) {
        gpio_set_level(PLACA_LCD_GPIO_BACKLIGHT, PLACA_LCD_BACKLIGHT_LIGADO);
    }
    return r;
}

static esp_err_t iniciar_painel_rgb(esp_lcd_panel_handle_t *painel)
{
    esp_lcd_rgb_panel_config_t cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = PLACA_LCD_PCLK_HZ,
            .h_res = PLACA_LCD_H_RES,
            .v_res = PLACA_LCD_V_RES,
            .hsync_pulse_width = PLACA_LCD_HSYNC_PULSO,
            .hsync_back_porch = PLACA_LCD_HSYNC_BACK_PORCH,
            .hsync_front_porch = PLACA_LCD_HSYNC_FRONT_PORCH,
            .vsync_pulse_width = PLACA_LCD_VSYNC_PULSO,
            .vsync_back_porch = PLACA_LCD_VSYNC_BACK_PORCH,
            .vsync_front_porch = PLACA_LCD_VSYNC_FRONT_PORCH,
            .flags = {
                .hsync_idle_low = PLACA_LCD_HSYNC_IDLE_LOW,
                .vsync_idle_low = PLACA_LCD_VSYNC_IDLE_LOW,
                .pclk_active_neg = PLACA_LCD_PCLK_ATIVO_NEG,
            },
        },
        .data_width = 16,           /* RGB565 paralelo */
        .bits_per_pixel = 16,
        .num_fbs = 2,               /* DUPLO framebuffer em PSRAM — anti-tearing */
        .bounce_buffer_size_px = PLACA_LCD_H_RES * PLACA_LCD_BOUNCE_LINHAS,
        .dma_burst_size = 64,       /* rajada de DMA da PSRAM (substitui psram_trans_align no IDF 5.4+) */
        .hsync_gpio_num = PLACA_LCD_GPIO_HSYNC,
        .vsync_gpio_num = PLACA_LCD_GPIO_VSYNC,
        .de_gpio_num = PLACA_LCD_GPIO_DE,
        .pclk_gpio_num = PLACA_LCD_GPIO_PCLK,
        .disp_gpio_num = -1,        /* painel sem pino de enable dedicado */
        .data_gpio_nums = {
            PLACA_LCD_GPIO_D0,  PLACA_LCD_GPIO_D1,  PLACA_LCD_GPIO_D2,
            PLACA_LCD_GPIO_D3,  PLACA_LCD_GPIO_D4,  PLACA_LCD_GPIO_D5,
            PLACA_LCD_GPIO_D6,  PLACA_LCD_GPIO_D7,  PLACA_LCD_GPIO_D8,
            PLACA_LCD_GPIO_D9,  PLACA_LCD_GPIO_D10, PLACA_LCD_GPIO_D11,
            PLACA_LCD_GPIO_D12, PLACA_LCD_GPIO_D13, PLACA_LCD_GPIO_D14,
            PLACA_LCD_GPIO_D15,
        },
        .flags = {
            .fb_in_psram = true,    /* 2 × 800×480×2 B = 1,5 MB: só cabe na PSRAM */
        },
    };

    esp_err_t r = esp_lcd_new_rgb_panel(&cfg, painel);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_rgb_panel: %s", esp_err_to_name(r));
        return r;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*painel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*painel));
    return ESP_OK;
}

/* Converte a faixa bruta do GT911 (480x272) para pixel de tela (800x480).
 * Chamado pelo esp_lcd_touch a cada leitura, antes de o LVGL ver o ponto. */
static void escalar_touch(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y,
                          uint16_t *forca, uint8_t *n_pontos, uint8_t max_pontos)
{
    (void)tp;
    (void)forca;
    (void)max_pontos;
    for (uint8_t i = 0; i < *n_pontos; i++) {
        uint32_t xe = (uint32_t)x[i] * PLACA_LCD_H_RES / PLACA_TOUCH_BRUTO_X_MAX;
        uint32_t ye = (uint32_t)y[i] * PLACA_LCD_V_RES / PLACA_TOUCH_BRUTO_Y_MAX;
        x[i] = (uint16_t)((xe < PLACA_LCD_H_RES) ? xe : PLACA_LCD_H_RES - 1);
        y[i] = (uint16_t)((ye < PLACA_LCD_V_RES) ? ye : PLACA_LCD_V_RES - 1);
    }
}

/* Reset do GT911 pelo RST e sonda de endereço — ver item 4 no topo */
static uint8_t resetar_e_achar_gt911(i2c_master_bus_handle_t barramento)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PLACA_TOUCH_GPIO_RST,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&cfg);
    gpio_set_level(PLACA_TOUCH_GPIO_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(GT911_RST_BAIXO_MS));
    gpio_set_level(PLACA_TOUCH_GPIO_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(GT911_ACORDAR_MS));

    if (i2c_master_probe(barramento, GT911_ENDERECO_PRIMARIO, GT911_TIMEOUT_SONDA_MS) == ESP_OK) {
        return GT911_ENDERECO_PRIMARIO;
    }
    if (i2c_master_probe(barramento, GT911_ENDERECO_ALTERNATIVO, GT911_TIMEOUT_SONDA_MS) == ESP_OK) {
        return GT911_ENDERECO_ALTERNATIVO;
    }
    return 0;
}

static esp_err_t iniciar_touch(esp_lcd_touch_handle_t *touch)
{
    i2c_master_bus_handle_t barramento = NULL;
    i2c_master_bus_config_t cfg_bus = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PLACA_TOUCH_GPIO_SDA,
        .scl_io_num = PLACA_TOUCH_GPIO_SCL,
        .glitch_ignore_cnt = 7,
        .flags = { .enable_internal_pullup = true },
    };
    esp_err_t r = i2c_new_master_bus(&cfg_bus, &barramento);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus: %s", esp_err_to_name(r));
        return r;
    }

    uint8_t endereco = resetar_e_achar_gt911(barramento);
    if (endereco == 0) {
        ESP_LOGE(TAG, "GT911 nao respondeu em 0x%02X nem em 0x%02X",
                 GT911_ENDERECO_PRIMARIO, GT911_ENDERECO_ALTERNATIVO);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "GT911 encontrado em 0x%02X", endereco);

    esp_lcd_panel_io_i2c_config_t cfg_io = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    cfg_io.dev_addr = endereco;
    cfg_io.scl_speed_hz = PLACA_TOUCH_I2C_HZ;

    esp_lcd_panel_io_handle_t io = NULL;
    r = esp_lcd_new_panel_io_i2c(barramento, &cfg_io, &io);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_io_i2c: %s", esp_err_to_name(r));
        return r;
    }

    esp_lcd_touch_config_t cfg_touch = {
        .x_max = PLACA_LCD_H_RES,
        .y_max = PLACA_LCD_V_RES,
        .rst_gpio_num = -1,               /* já resetado acima — não repetir */
        .int_gpio_num = PLACA_TOUCH_GPIO_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .process_coordinates = escalar_touch,
    };
    r = esp_lcd_touch_new_i2c_gt911(io, &cfg_touch, touch);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_touch_new_i2c_gt911: %s", esp_err_to_name(r));
    }
    return r;
}

esp_err_t display_iniciar(void)
{
    esp_lcd_panel_handle_t painel = NULL;
    esp_lcd_touch_handle_t touch = NULL;

    ESP_ERROR_CHECK(iniciar_backlight());
    ESP_ERROR_CHECK(iniciar_painel_rgb(&painel));
    ESP_ERROR_CHECK(iniciar_touch(&touch));

    /* esp_lvgl_port: cria a task do LVGL, o tick e o registro de displays */
    lvgl_port_cfg_t cfg_port = ESP_LVGL_PORT_INIT_CONFIG();
    cfg_port.task_affinity = LVGL_TASK_CORE;   /* LVGL no core 1; rede no core 0 */
    cfg_port.task_priority = LVGL_TASK_PRIORIDADE;
    cfg_port.task_stack = LVGL_TASK_STACK;
    ESP_ERROR_CHECK(lvgl_port_init(&cfg_port));

    lvgl_port_display_cfg_t cfg_disp = {
        .panel_handle = painel,
        .buffer_size = PLACA_LCD_H_RES * PLACA_LCD_V_RES,
        .double_buffer = true,
        .hres = PLACA_LCD_H_RES,
        .vres = PLACA_LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        .flags = {
            .buff_dma = false,
            .buff_spiram = false,
            /* full_refresh + 2 framebuffers do painel = troca no vsync,
             * zero tearing (modo anti-tearing do esp_lvgl_port p/ RGB) */
            .full_refresh = true,
            .direct_mode = false,
            .swap_bytes = false,
        },
    };
    lvgl_port_display_rgb_cfg_t cfg_rgb = {
        .flags = {
            .bb_mode = true,        /* casa com o bounce buffer do painel */
            .avoid_tearing = true,  /* LVGL desenha nos FBs do próprio painel */
        },
    };
    lv_display_t *disp = lvgl_port_add_disp_rgb(&cfg_disp, &cfg_rgb);
    if (disp == NULL) {
        ESP_LOGE(TAG, "lvgl_port_add_disp_rgb falhou");
        return ESP_FAIL;
    }

    lvgl_port_touch_cfg_t cfg_touch_port = {
        .disp = disp,
        .handle = touch,
    };
    if (lvgl_port_add_touch(&cfg_touch_port) == NULL) {
        ESP_LOGE(TAG, "lvgl_port_add_touch falhou");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "painel %dx%d no ar, LVGL no core %d",
             PLACA_LCD_H_RES, PLACA_LCD_V_RES, LVGL_TASK_CORE);
    return ESP_OK;
}
