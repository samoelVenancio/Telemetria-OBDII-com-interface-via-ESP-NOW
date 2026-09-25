/**
 * enlace_espnow.c — Recepção da telemetria por ESP-NOW no Módulo B.
 *
 * O callback de recepção é DELIBERADAMENTE mínimo. Ele roda na task do Wi-Fi
 * (core 0, prioridade alta) e por isso:
 *   - NÃO chama nada de LVGL (não é thread-safe; travaria o firmware);
 *   - NÃO loga em nível INFO (log em flash bloqueia);
 *   - só valida magic/versão/CRC e faz memcpy sob seção crítica, tudo dentro
 *     de estado_processar_rx(), e retorna.
 * Quem consome o dado é o lv_timer da UI, já no contexto certo (core 1).
 *
 * O caminho inverso (tela -> Módulo A) é seguro: a action do botão roda na
 * task do LVGL e chama esp_now_send(), que é thread-safe. Proibido é só a
 * task do Wi-Fi chamar LVGL, não o LVGL chamar o Wi-Fi.
 */
#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "telemetria_protocolo.h"
#include "estado.h"
#include "enlace_espnow.h"

static const char *TAG = "enlace";

/* Comandos vão em broadcast, como a telemetria (ver enlace do Módulo A).
 * TODO: trocar pelo MAC do Módulo A para unicast com ACK. */
static const uint8_t s_mac_modulo_a[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t s_seq_cmd = 0;

static void ao_receber(const esp_now_recv_info_t *info, const uint8_t *dados, int tamanho)
{
    /* RSSI vem do metadado de recepção do rádio (não é campo do pacote) */
    int rssi = (info != NULL && info->rx_ctrl != NULL) ? info->rx_ctrl->rssi : 0;
    estado_processar_rx(dados, tamanho, rssi);
    /* Nada de LVGL aqui. Nada mesmo. Ver comentário no topo do arquivo. */
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
    /* Power save desligado: com PS ligado o rádio dorme entre beacons e
     * perderia pacotes ESP-NOW broadcast — o enlace "caindo" aleatoriamente. */
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(ao_receber));

    esp_now_peer_info_t par = { 0 };
    memcpy(par.peer_addr, s_mac_modulo_a, sizeof(s_mac_modulo_a));
    par.channel = TELEM_CANAL_WIFI;
    par.ifidx = WIFI_IF_STA;
    par.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&par));

    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
    ESP_LOGI(TAG, "ouvindo ESP-NOW no canal %d — MAC deste modulo (para o "
                  "unicast do Modulo A): %02X:%02X:%02X:%02X:%02X:%02X",
             TELEM_CANAL_WIFI, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return ESP_OK;
}

esp_err_t enlace_espnow_enviar_comando(uint8_t comando, uint16_t valor)
{
    telem_comando_t cmd = {
        .magic = TELEM_MAGIC_CMD,
        .versao = TELEM_VERSAO,
        .comando = comando,
        .valor = valor,
        .seq = s_seq_cmd++,
    };
    cmd.crc16 = telem_crc16_comando(&cmd);

    esp_err_t r = esp_now_send(s_mac_modulo_a, (const uint8_t *)&cmd, sizeof(cmd));
    ESP_LOGI(TAG, "comando %u (valor %u) enviado: %s", comando, valor, esp_err_to_name(r));
    return r;
}
