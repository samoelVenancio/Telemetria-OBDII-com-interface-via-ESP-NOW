/**
 * energia.h — Gestão de energia do Módulo A (alimentado pelo pino 16 do OBD2).
 */
#pragma once

#include <stdint.h>

/* Loga a causa do último wakeup (boot frio × timer do deep sleep) */
void energia_iniciar(void);

/* Entra em deep sleep imediatamente, com wakeup por timer. Não retorna. */
void energia_dormir_agora(void);

/* Chamado no laço de aquisição: se o barramento CAN está em silêncio há mais
 * que o limite, dorme. Recebe o timestamp do último quadro visto. */
void energia_avaliar(int64_t ultima_atividade_can_ms);
