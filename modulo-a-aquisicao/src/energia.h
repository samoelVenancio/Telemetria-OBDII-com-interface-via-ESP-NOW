/**
 * energia.h — Gestão de energia do Módulo A (alimentado pelo pino 16 do OBD2).
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Loga a causa do último wakeup (boot frio × timer do deep sleep) */
void energia_iniciar(void);

/* Trava o TX do CAN em recessivo e entra em deep sleep, com wakeup por
 * timer. A duração cresce se os despertares seguidos não acharem o ECM
 * (ver energia.c). Não retorna. */
void energia_dormir_agora(void);

/* A escuta do boot achou o barramento totalmente quieto (rede do carro em
 * repouso): volta ao intervalo curto de despertar. */
void energia_rede_em_repouso(void);

/* Chamado no laço de aquisição: dorme se o ECM está calado há mais que o
 * limite. 'ultima_resposta_ms' = último quadro do ECM (ou a entrada no modo
 * normal, se ele nunca respondeu); 'ecm_respondeu' = se já respondeu alguma
 * vez neste boot. */
void energia_avaliar(int64_t ultima_resposta_ms, bool ecm_respondeu);
