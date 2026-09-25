/**
 * enlace_espnow.c — Enlace ESP-NOW do Módulo A.
 *
 * Decisões:
 *  - Canal Wi-Fi fixo (TELEM_CANAL_WIFI no header comum): nenhum dos módulos
 *    se associa a AP, então o canal precisa ser combinado a priori.
 *  - Destino em broadcast por padrão: dispensa descoberta de peer e funciona
 *    de primeira na bancada. Broadcast não tem ACK de enlace — a detecção de
 *    perda é feita no receptor pelo campo seq do pacote, que já existe para
 *    isso. TODO: trocar pelo MAC real do Módulo B (unicast) quando o par
 *    estiver definido, o que habilita retry/ACK de camada MAC.
 *  - Power save desligado: o módulo é alimentado pelo OBD2, latência importa
 *    mais que corrente aqui (a economia de verdade é o deep sleep, energia.c).
 *  - Comandos do Módulo B (canal de retorno): o callback de recepção roda na
 *    task do Wi-Fi, então ele só valida e deixa o pedido guardado sob
 *    spinlock. Quem age (gravar NVS, reiniciar) é a task de aquisição, fora
 *    do contexto do Wi-Fi.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "enlace_espnow.h"

static const char *TAG = "enlace";

/* TODO: substituir pelo MAC do Módulo B para enlace unicast com ACK */
static uint8_t s_mac_destino[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static uint16_t s_seq = 0;
static bool s_pronto = false;

static uint16_t s_taxa_pedida_kbps = 0;
static portMUX_TYPE s_trava_cmd = portMUX_INITIALIZER_UNLOCKED;

static void ao_receber(const esp_now_recv_info_t *info, const uint8_t *dados, int tamanho)
{
    (void)info;
    if (tamanho != (int)sizeof(telem_comando_t)) {
        return; /* não é comando (ex.: telemetria de outro módulo A no ar) */
    }
    telem_comando_t c;
    memcpy(&c, dados, sizeof(c)); /* buffer do rádio sem alinhamento garantido */
    if (!telem_comando_valido(&c)) {
        return;
    }
    if (c.comando == TELEM_CMD_DEFINIR_CAN_KBPS) {
        portENTER_CRITICAL(&s_trava_cmd);
        s_taxa_pedida_kbps = c.valor;
        portEXIT_CRITICAL(&s_trava_cmd);
    }
}

esp_err_t enlace_espnow_iniciar(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(TELEM_CANAL_WIFI, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(ao_receber));

    esp_now_peer_info_t par = { 0 };
    memcpy(par.peer_addr, s_mac_destino, sizeof(s_mac_destino));
    par.channel = TELEM_CANAL_WIFI;
    par.ifidx = WIFI_IF_STA;
    par.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&par));

    s_pronto = true;
    ESP_LOGI(TAG, "ESP-NOW pronto no canal %d (pacote de %u bytes)",
             TELEM_CANAL_WIFI, (unsigned)sizeof(telem_pacote_t));
    return ESP_OK;
}

esp_err_t enlace_espnow_enviar(telem_pacote_t *pacote)
{
    if (!s_pronto) {
        return ESP_ERR_INVALID_STATE;
    }
    pacote->magic = TELEM_MAGIC;
    pacote->versao = TELEM_VERSAO;
    pacote->seq = s_seq++;
    /* CRC por último: cobre tudo que veio antes dele no pacote */
    pacote->crc16 = telem_crc16_pacote(pacote);

    esp_err_t r = esp_now_send(s_mac_destino, (const uint8_t *)pacote, sizeof(*pacote));
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "esp_now_send: %s", esp_err_to_name(r));
    }
    return r;
}

uint16_t enlace_espnow_consumir_taxa_pedida(void)
{
    portENTER_CRITICAL(&s_trava_cmd);
    uint16_t kbps = s_taxa_pedida_kbps;
    s_taxa_pedida_kbps = 0;
    portEXIT_CRITICAL(&s_trava_cmd);
    return kbps;
}
