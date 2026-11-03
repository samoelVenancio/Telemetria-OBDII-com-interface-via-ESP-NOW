/**
 * varredura.c — ver varredura.h.
 */
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

#include "can_obd2.h"
#include "pids.h"
#include "varredura.h"

static const char *TAG = "varredura";

#define PID_SUPORTE_BASE   0x00u  /* 0x00, 0x20, 0x40, 0x60: um bitmap cada */
#define PID_SONDAS_O2      0x13u  /* A: b0..b3 banco 1 sensores 1..4, b4..b7 banco 2 */
#define FAIXAS_SUPORTE     4u
#define TENTATIVAS         2u

/* Requisita um PID com retentativa. Espaça ANTES de cada requisição, então
 * duas chamadas seguidas nunca saem coladas. */
static bool pedir(uint8_t pid, uint8_t *resp, size_t minimo,
                  uint32_t intervalo_ms, uint32_t timeout_ms)
{
    for (unsigned t = 0; t < TENTATIVAS; t++) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(intervalo_ms));
        size_t n = 0;
        if (can_obd2_requisitar_pid(pid, resp, &n, timeout_ms) == ESP_OK && n >= minimo) {
            return true;
        }
    }
    return false;
}

static void logar(const telem_suporte_t *s)
{
    char lista[3 * 0x80 + 1];
    size_t pos = 0;
    unsigned total = 0;
    for (unsigned pid = 1; pid <= 0x80; pid++) {
        /* os PIDs de bitmap (0x20, 0x40...) só encadeiam a varredura */
        if ((pid % 0x20u) != 0 && telem_pid_suportado(s, (uint8_t)pid)) {
            pos += (size_t)snprintf(lista + pos, sizeof(lista) - pos, " %02X", pid);
            total++;
        }
    }
    lista[pos] = '\0';
    ESP_LOGI(TAG, "%u PIDs suportados:%s", total, lista);
    ESP_LOGI(TAG, "avanco 0x0E: %s | sonda B1S1 0x14: %s | B1S2 0x15: %s | "
                  "lambda banda larga 0x24/0x34: %s | sondas (0x13) 0x%02X",
             telem_pid_suportado(s, 0x0E) ? "sim" : "nao",
             telem_pid_suportado(s, 0x14) ? "sim" : "nao",
             telem_pid_suportado(s, 0x15) ? "sim" : "nao",
             (telem_pid_suportado(s, 0x24) || telem_pid_suportado(s, 0x34)) ? "sim" : "nao",
             s->sondas);
}

void varredura_executar(telem_suporte_t *s, uint32_t intervalo_ms, uint32_t timeout_ms)
{
    memset(s->mapa, 0, sizeof(s->mapa));
    s->sondas = 0;

    for (unsigned k = 0; k < FAIXAS_SUPORTE; k++) {
        uint8_t pid = (uint8_t)(PID_SUPORTE_BASE + k * 0x20u);
        uint8_t r[8];
        if (!pedir(pid, r, 4, intervalo_ms, timeout_ms)) {
            /* Sem o 0x00 não há mapa nenhum; no meio da cadeia, o que já veio
             * pode estar incompleto. Nos dois casos fica para a próxima. */
            s->estado = TELEM_VARREDURA_SEM_ECM;
            ESP_LOGW(TAG, "ECM nao respondeu ao PID 0x%02X — varredura adiada", pid);
            return;
        }
        s->mapa[k] = ((uint32_t)r[0] << 24) | ((uint32_t)r[1] << 16) |
                     ((uint32_t)r[2] << 8) | r[3];
        if ((s->mapa[k] & 1u) == 0) {
            break; /* bit da base seguinte apagado: fim da cadeia */
        }
    }

    if (telem_pid_suportado(s, PID_SONDAS_O2)) {
        uint8_t r[8];
        if (pedir(PID_SONDAS_O2, r, 1, intervalo_ms, timeout_ms)) {
            s->sondas = r[0];
        }
    }

    s->estado = TELEM_VARREDURA_COMPLETA;
    logar(s);
}

/* Liga o bit do PID no mapa (mesma convenção de telem_pid_suportado) */
static void marcar(telem_suporte_t *s, uint8_t pid)
{
    uint8_t i = (uint8_t)(pid - 1u);
    s->mapa[i / 32u] |= 1u << (31u - (i % 32u));
}

void varredura_demo(telem_suporte_t *s)
{
    static const uint8_t PIDS[] = {
        PID_TEMP_ARREF, PID_MAP, PID_RPM, PID_VELOCIDADE, PID_IAT,
        PID_NIVEL, PID_TENSAO, PID_ETANOL,
        0x20, 0x40, /* encadeamento dos bitmaps */
    };
    memset(s->mapa, 0, sizeof(s->mapa));
    for (size_t i = 0; i < sizeof(PIDS); i++) {
        marcar(s, PIDS[i]);
    }
    s->sondas = 0;
    s->estado = TELEM_VARREDURA_DEMO;
}
