/**
 * estado.c — Ponte thread-safe entre o ESP-NOW e a UI.
 *
 * POR QUE A SEÇÃO CRÍTICA (e por que o callback não toca em LVGL):
 * o callback de recepção do ESP-NOW roda na task interna do Wi-Fi, no core 0,
 * com prioridade alta. O LVGL roda na própria task no core 1 e NÃO é
 * thread-safe: chamar qualquer lv_* de outro contexto corrompe as listas
 * internas e trava o firmware de forma intermitente — é a causa número um de
 * travamento nesse tipo de projeto. A regra aqui é estrutural: o callback só
 * valida e copia 33 bytes sob spinlock; quem lê é o lv_timer, já dentro da
 * task do LVGL. O spinlock (portMUX) funciona entre cores e a seção crítica
 * dura menos de um microssegundo — mutex seria mais caro que o trabalho.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "estado.h"

static telem_pacote_t s_pacote;
static estado_enlace_t s_enlace;
static bool s_ja_recebeu = false;
static portMUX_TYPE s_trava = portMUX_INITIALIZER_UNLOCKED;

static int64_t agora_ms(void)
{
    return esp_timer_get_time() / 1000;
}

void estado_iniciar(void)
{
    memset(&s_pacote, 0, sizeof(s_pacote));
    memset(&s_enlace, 0, sizeof(s_enlace));
    s_ja_recebeu = false;
}

bool estado_processar_rx(const uint8_t *dados, int tamanho, int rssi_dbm)
{
    /* Validação fora da seção crítica: CRC sobre 33 bytes é barato, mas não
     * precisa rodar com interrupções travadas. */
    if (tamanho != (int)sizeof(telem_pacote_t)) {
        portENTER_CRITICAL(&s_trava);
        s_enlace.invalidos++;
        portEXIT_CRITICAL(&s_trava);
        return false;
    }

    /* memcpy para local alinhado antes de ler campos: o buffer do rádio não
     * tem alinhamento garantido e a struct é packed. */
    telem_pacote_t p;
    memcpy(&p, dados, sizeof(p));

    if (!telem_pacote_valido(&p)) {
        portENTER_CRITICAL(&s_trava);
        s_enlace.invalidos++;
        portEXIT_CRITICAL(&s_trava);
        return false;
    }

    int64_t agora = agora_ms();

    portENTER_CRITICAL(&s_trava);
    if (s_ja_recebeu) {
        /* seq é uint16 com wraparound; a aritmética sem sinal já resolve */
        uint16_t salto = (uint16_t)(p.seq - s_pacote.seq);
        if (salto > 1) {
            s_enlace.perdidos += salto - 1;
        }
    }
    s_pacote = p;
    s_enlace.rssi_dbm = (int8_t)rssi_dbm;
    s_enlace.recebidos++;
    s_enlace.ultimo_rx_ms = agora;
    s_ja_recebeu = true;
    portEXIT_CRITICAL(&s_trava);

    return true;
}

bool estado_copiar(telem_pacote_t *pacote, estado_enlace_t *enlace)
{
    portENTER_CRITICAL(&s_trava);
    bool tem = s_ja_recebeu;
    if (pacote != NULL) {
        *pacote = s_pacote;
    }
    if (enlace != NULL) {
        *enlace = s_enlace;
    }
    portEXIT_CRITICAL(&s_trava);
    return tem;
}

bool estado_enlace_ok(void)
{
    portENTER_CRITICAL(&s_trava);
    bool tem = s_ja_recebeu;
    int64_t ultimo = s_enlace.ultimo_rx_ms;
    portEXIT_CRITICAL(&s_trava);
    return tem && (agora_ms() - ultimo) <= ESTADO_ENLACE_TIMEOUT_MS;
}
