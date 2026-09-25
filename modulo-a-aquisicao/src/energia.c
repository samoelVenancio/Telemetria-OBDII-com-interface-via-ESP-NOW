/**
 * energia.c — Deep sleep quando o carro dorme.
 *
 * O módulo é alimentado pelo pino 16 do J1962 (12 V PERMANENTE, direto da
 * bateria), então ele continua energizado com o carro desligado. A política:
 *  - Barramento CAN em silêncio por 30 s -> carro dormiu -> deep sleep.
 *  - Wakeup por timer a cada 60 s para reescutar o barramento (o boot já
 *    começa em listen-only, então a reavaliação é o próprio fluxo normal).
 *
 * PISO DE CONSUMO — importante para a banca: o regulador buck LM2596 tem
 * corrente quiescente de ~5 mA drenando a bateria SEMPRE, mesmo com o ESP32
 * em deep sleep (~5 µA). Esse piso é do hardware e NÃO é eliminável por
 * software; reduzir de verdade exigiria trocar o LM2596 por um buck de baixa
 * quiescente (ex.: classe 10 µA) ou um circuito de corte. ~5 mA drenam uma
 * bateria de 60 Ah em ~500 dias — aceitável para o escopo, mas deve ser dito.
 *
 * TODO fase 2: wakeup por atividade no pino RX do CAN (GPIO wakeup do C3)
 * em vez de timer, zerando a janela de latência de 60 s.
 */
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#include "energia.h"

static const char *TAG = "energia";

#define SILENCIO_PARA_DORMIR_MS  30000       /* 30 s sem quadro CAN = carro dormiu */
#define DURACAO_SONO_US          (60ULL * 1000ULL * 1000ULL) /* reavalia a cada 60 s */

void energia_iniciar(void)
{
    esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause();
    if (causa == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "acordou do deep sleep (timer de 60 s) para reescutar o barramento");
    } else {
        ESP_LOGI(TAG, "boot frio");
    }
}

void energia_dormir_agora(void)
{
    ESP_LOGI(TAG, "entrando em deep sleep por %llu s",
             (unsigned long long)(DURACAO_SONO_US / 1000000ULL));
    esp_sleep_enable_timer_wakeup(DURACAO_SONO_US);
    esp_deep_sleep_start(); /* não retorna */
}

void energia_avaliar(int64_t ultima_atividade_can_ms)
{
    int64_t agora_ms = esp_timer_get_time() / 1000;
    if (agora_ms - ultima_atividade_can_ms > SILENCIO_PARA_DORMIR_MS) {
        ESP_LOGW(TAG, "CAN em silencio ha mais de %d ms — carro dormiu",
                 SILENCIO_PARA_DORMIR_MS);
        energia_dormir_agora();
    }
}
