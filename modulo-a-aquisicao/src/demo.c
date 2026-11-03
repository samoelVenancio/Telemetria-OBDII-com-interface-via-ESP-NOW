/**
 * demo.c — ver demo.h.
 *
 * Os números imitam o New Fiesta 1.6: marcha lenta ~850 rpm, 4ª marcha a
 * 80 km/h ~2400 rpm. A 1ª marcha passa de 3000 rpm de propósito, para o
 * aviso ECO e o de motor frio (antes de aquecer) aparecerem no Módulo B.
 */
#include "demo.h"

#define CICLO_MS             90000
#define AQUECIMENTO_MS       180000   /* 25 °C -> 90 °C */
#define PERIODO_ALARME_MS    300000   /* um superaquecimento a cada 5 min */
#define RPM_MARCHA_LENTA     850

/* Ruído barato e determinístico (LCG): -amplitude..+amplitude */
static int32_t ruido(int32_t amplitude)
{
    static uint32_t semente = 12345u;
    semente = semente * 1103515245u + 12345u;
    return (int32_t)((semente >> 16) % (uint32_t)(2 * amplitude + 1)) - amplitude;
}

/* rpm a partir da velocidade, com a marcha escolhida pela própria velocidade */
static int32_t rpm_da_velocidade(int32_t kmh)
{
    int32_t rpm_por_kmh = (kmh < 20) ? 160 : (kmh < 40) ? 70 : (kmh < 60) ? 45 : 30;
    int32_t rpm = kmh * rpm_por_kmh;
    return (rpm < RPM_MARCHA_LENTA) ? RPM_MARCHA_LENTA : rpm;
}

/* Temperatura do líquido em décimos de °C */
static int32_t temperatura_motor(int64_t t_ms)
{
    if (t_ms < AQUECIMENTO_MS) {
        return 250 + (int32_t)(650 * t_ms / AQUECIMENTO_MS);
    }
    /* Depois de aquecido: 90 °C, com um episódio de superaquecimento
     * (sobe 30 s, fica 30 s em 108 °C, desce 30 s) */
    int32_t u = (int32_t)((t_ms - AQUECIMENTO_MS) % PERIODO_ALARME_MS);
    if (u >= 120000 && u < 150000) {
        return 900 + (u - 120000) * 180 / 30000;
    }
    if (u >= 150000 && u < 180000) {
        return 1080;
    }
    if (u >= 180000 && u < 210000) {
        return 1080 - (u - 180000) * 180 / 30000;
    }
    return 900;
}

void demo_gerar(int64_t t_ms, dados_veiculo_t *d)
{
    int32_t c = (int32_t)(t_ms % CICLO_MS);
    int32_t kmh;
    int32_t map;
    if (c < 10000) {                 /* marcha lenta */
        kmh = 0;
        map = 33;
    } else if (c < 30000) {          /* aceleração 0 -> 80 km/h em 20 s */
        kmh = (c - 10000) * 80 / 20000;
        map = 78;
    } else if (c < 60000) {          /* cruzeiro */
        kmh = 80;
        map = 45;
    } else if (c < 75000) {          /* frenagem 80 -> 0 em 15 s */
        kmh = 80 - (c - 60000) * 80 / 15000;
        map = 25;
    } else {                         /* parado de novo */
        kmh = 0;
        map = 33;
    }

    int32_t rpm = (kmh == 0) ? RPM_MARCHA_LENTA : rpm_da_velocidade(kmh);
    rpm += ruido(rpm / 50);          /* ~2 % */

    int32_t nivel = 75 - (int32_t)(t_ms / 120000);   /* -1 % a cada 2 min */
    if (nivel < 5) {
        nivel = 5;
    }

    d->rpm = (uint16_t)rpm;                                  d->rpm_valido = true;
    d->velocidade_kmh = (uint8_t)kmh;                        d->velocidade_valida = true;
    d->temp_arref_d = (int16_t)(temperatura_motor(t_ms) + ruido(2));
    d->temp_arref_valida = true;
    d->iat_d = (int16_t)(320 + ruido(5));                    d->iat_valida = true;
    d->map_kpa = (uint16_t)(map + ruido(2));                 d->map_valido = true;
    d->etanol_pct = 27;                                      d->etanol_valido = true;
    d->nivel_pct = (uint8_t)nivel;                           d->nivel_valido = true;
    d->tensao_c = (uint16_t)(((kmh == 0) ? 1380 : 1410) + ruido(5));
    d->tensao_valida = true;
}
