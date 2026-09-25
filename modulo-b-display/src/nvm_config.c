/**
 * nvm_config.c — Persistência de configuração na NVS.
 *
 * Estratégia: uma chave por campo (em vez de blob) para que acrescentar campo
 * novo em versão futura não invalide os já gravados. Campo ausente no
 * primeiro boot recebe o padrão e segue o jogo — a NVS vazia nunca é erro.
 *
 * O VE fica espelhado aqui porque a tela de configuração dele vive no display;
 * a sincronização B -> A (para o cálculo de consumo de verdade, que roda no
 * Módulo A) é fase 2 — exige canal de retorno no ESP-NOW.
 */
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "nvm_config.h"

static const char *TAG = "nvm_config";

#define NVS_NAMESPACE  "telemetria"

/* Chaves — máx. 15 caracteres (limite da NVS) */
#define CHAVE_LIMIAR    "limiar_temp_d"
#define CHAVE_HISTERESE "histerese_d"
#define CHAVE_JANELA    "janela_media"
#define CHAVE_TANQUE    "tanque_dl"
#define CHAVE_VE        "ve_milesimos"

/* Padrões de fábrica */
#define PADRAO_LIMIAR_TEMP_D  1050   /* 105,0 °C — acima da faixa normal (~90-100) do 1.6 Sigma */
#define PADRAO_HISTERESE_D    30     /* 3,0 °C */
#define PADRAO_JANELA_MEDIA   8      /* 8 amostras a 10 Hz ~= 0,8 s de suavização */
#define PADRAO_TANQUE_DL      480    /* 48,0 L — TODO: confirmar no manual do New Fiesta */
#define PADRAO_VE_MILESIMOS   850    /* VE 0,850 — mesmo padrão do Módulo A */

static nvm_config_t s_cfg = {
    .limiar_temp_d = PADRAO_LIMIAR_TEMP_D,
    .histerese_d = PADRAO_HISTERESE_D,
    .janela_media = PADRAO_JANELA_MEDIA,
    .tanque_dl = PADRAO_TANQUE_DL,
    .ve_milesimos = PADRAO_VE_MILESIMOS,
};

void nvm_config_iniciar(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "NVS vazia — usando padroes de fabrica");
        return;
    }
    /* Cada get que falhar mantém o padrão já presente em s_cfg */
    nvs_get_i16(h, CHAVE_LIMIAR, &s_cfg.limiar_temp_d);
    nvs_get_i16(h, CHAVE_HISTERESE, &s_cfg.histerese_d);
    nvs_get_u8(h, CHAVE_JANELA, &s_cfg.janela_media);
    nvs_get_u16(h, CHAVE_TANQUE, &s_cfg.tanque_dl);
    nvs_get_u16(h, CHAVE_VE, &s_cfg.ve_milesimos);
    nvs_close(h);

    ESP_LOGI(TAG, "config: limiar %d.%d C, hist %d.%d C, janela %u, tanque %u.%u L, VE %u/1000",
             s_cfg.limiar_temp_d / 10, s_cfg.limiar_temp_d % 10,
             s_cfg.histerese_d / 10, s_cfg.histerese_d % 10,
             (unsigned)s_cfg.janela_media,
             s_cfg.tanque_dl / 10, s_cfg.tanque_dl % 10,
             (unsigned)s_cfg.ve_milesimos);
}

const nvm_config_t *nvm_config(void)
{
    return &s_cfg;
}

esp_err_t nvm_config_salvar(const nvm_config_t *nova)
{
    if (nova->limiar_temp_d < NVM_LIMIAR_TEMP_MIN_D ||
        nova->limiar_temp_d > NVM_LIMIAR_TEMP_MAX_D ||
        nova->histerese_d < NVM_HISTERESE_MIN_D ||
        nova->histerese_d > NVM_HISTERESE_MAX_D ||
        nova->janela_media < NVM_JANELA_MIN ||
        nova->janela_media > NVM_JANELA_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (r != ESP_OK) {
        return r;
    }
    nvs_set_i16(h, CHAVE_LIMIAR, nova->limiar_temp_d);
    nvs_set_i16(h, CHAVE_HISTERESE, nova->histerese_d);
    nvs_set_u8(h, CHAVE_JANELA, nova->janela_media);
    nvs_set_u16(h, CHAVE_TANQUE, nova->tanque_dl);
    nvs_set_u16(h, CHAVE_VE, nova->ve_milesimos);
    r = nvs_commit(h);
    nvs_close(h);

    if (r == ESP_OK) {
        s_cfg = *nova;
        ESP_LOGI(TAG, "configuracao gravada");
    }
    return r;
}
