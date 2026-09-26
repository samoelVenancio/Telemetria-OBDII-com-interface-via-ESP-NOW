/**
 * energia.c — Deep sleep quando o carro dorme.
 *
 * O módulo é alimentado pelo pino 16 do J1962 (12 V PERMANENTE, direto da
 * bateria), então continua energizado com o carro desligado. A política:
 *
 *  - Carro ligado = o ECM responde aos PIDs. Se ele fica calado, dorme:
 *      · 10 s de silêncio depois de já ter respondido (a chave foi desligada);
 *      · 5 s logo após o boot se ele nunca respondeu (o barramento está vivo,
 *        mas o ECM não — típico dos minutos depois de desligar a chave, com
 *        painel/BCM ainda conversando).
 *  - Wakeup por timer para reescutar o barramento. O boot começa em
 *    listen-only, então reavaliar é o próprio fluxo normal de inicialização.
 *
 * Por que dormir rápido e espaçar os despertares: depois que a chave é
 * desligada, os módulos do carro ficam alguns minutos acordados antes de a
 * rede CAN entrar em repouso. Se o Módulo A acordasse e ficasse mandando
 * requisições nesse período, o tráfego de diagnóstico poderia MANTER a rede
 * acordada e descarregar a bateria. Por isso, cada despertar em que o ECM
 * não aparece aumenta o intervalo seguinte (60 s -> 5 min), e o contador
 * zera assim que o ECM volta a responder (carro ligado de novo).
 *
 * Antes de dormir o TX do CAN é travado em recessivo (can_obd2_preparar_sono):
 * sem isso o pino flutua no deep sleep e o transceptor pode forçar dominante
 * no barramento do carro.
 *
 * PISO DE CONSUMO — importante para a banca: com o ESP32-C3 em deep sleep
 * (~5 µA), o que sobra é hardware sempre alimentado — corrente de repouso do
 * regulador TRACO, LED de alimentação da placa SuperMini, regulador LDO da
 * própria placa e o transceptor SN65HVD230 (fora de standby, porque o pino Rs
 * está fixo em GND no módulo). Isso NÃO é eliminável por software.
 * TODO: medir com multímetro em série no pino 16, carro desligado há 5 min.
 *
 * TODO fase 2: wakeup por atividade no pino RX do CAN (GPIO wakeup do C3)
 * em vez de timer, zerando a latência de acordar.
 */
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_attr.h"

#include "can_obd2.h"
#include "energia.h"

static const char *TAG = "energia";

#define SILENCIO_ECM_APOS_RESPOSTA_MS  10000  /* ECM calado 10 s = chave desligada */
#define SILENCIO_ECM_SEM_RESPOSTA_MS   5000   /* ECM nunca respondeu neste boot */
#define SONO_CURTO_US   (60ULL * 1000ULL * 1000ULL)       /* 60 s */
#define SONO_LONGO_US   (5ULL * 60ULL * 1000ULL * 1000ULL) /* 5 min */
#define DESPERTARES_ANTES_DO_SONO_LONGO  3

/* Sobrevive ao deep sleep (memória RTC); zera em boot frio */
static RTC_DATA_ATTR uint32_t s_despertares_sem_ecm = 0;

void energia_iniciar(void)
{
    esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause();
    if (causa == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "acordou do deep sleep para reescutar o barramento "
                      "(%lu despertar(es) seguido(s) sem ECM)",
                 (unsigned long)s_despertares_sem_ecm);
    } else {
        s_despertares_sem_ecm = 0;
        ESP_LOGI(TAG, "boot frio");
    }
}

void energia_rede_em_repouso(void)
{
    /* Barramento totalmente quieto: a rede do carro já dormiu. Escutar a cada
     * 60 s não a acorda (listen-only não transmite), então volta ao intervalo
     * curto — e o painel mostra dados em até 60 s depois de ligar o carro. */
    s_despertares_sem_ecm = 0;
}

void energia_dormir_agora(void)
{
    uint64_t duracao_us = (s_despertares_sem_ecm >= DESPERTARES_ANTES_DO_SONO_LONGO)
                          ? SONO_LONGO_US : SONO_CURTO_US;

    can_obd2_preparar_sono(); /* TX travado em recessivo — segurança do barramento */

    ESP_LOGI(TAG, "entrando em deep sleep por %llu s",
             (unsigned long long)(duracao_us / 1000000ULL));
    esp_sleep_enable_timer_wakeup(duracao_us);
    esp_deep_sleep_start(); /* não retorna */
}

void energia_avaliar(int64_t ultima_resposta_ms, bool ecm_respondeu)
{
    int64_t limite_ms = ecm_respondeu ? SILENCIO_ECM_APOS_RESPOSTA_MS
                                      : SILENCIO_ECM_SEM_RESPOSTA_MS;
    int64_t agora_ms = esp_timer_get_time() / 1000;
    if (agora_ms - ultima_resposta_ms <= limite_ms) {
        return;
    }

    if (ecm_respondeu) {
        /* Carro estava ligado e foi desligado agora: volta ao intervalo curto */
        s_despertares_sem_ecm = 0;
        ESP_LOGW(TAG, "ECM calado ha %lld ms — chave desligada", (long long)limite_ms);
    } else {
        s_despertares_sem_ecm++;
        ESP_LOGW(TAG, "barramento vivo mas ECM nao responde (%lu seguido(s)) — "
                      "carro desligado, rede ainda acordada",
                 (unsigned long)s_despertares_sem_ecm);
    }
    energia_dormir_agora();
}
