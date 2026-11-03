/**
 * varredura.h — Varredura dos PIDs que o ECM suporta (J1979, Modo 01).
 *
 * O ECM responde ao PID 0x00 com um bitmap de 32 bits dizendo quais dos PIDs
 * 0x01..0x20 ele tem; o último bit (0x20) diz se vale perguntar o 0x20, que
 * cobre 0x21..0x40, e assim por diante até 0x60. Com o mapa na mão, o PID
 * 0x13 (se existir) diz quais sondas O2 estão instaladas.
 *
 * Só roda quando a tela pede (botão VARRER AGORA, TELEM_CMD_VARRER_PIDS).
 * O resultado vai para o Módulo B (telem_suporte_t), que mostra na tela
 * "PIDS DO ECM" o que dá para ler deste carro — avanço de ignição, sondas,
 * lambda de banda larga etc. — sem precisar de scanner externo.
 */
#pragma once

#include <stdint.h>
#include "telemetria_protocolo.h"

/* Faz a varredura. Bloqueia de ~0,5 s a ~2,5 s (até 5 requisições, cada uma
 * com uma retentativa), espaçadas por 'intervalo_ms' — nunca em rajada.
 * Chamar da MESMA task que faz as requisições de PID (e que está inscrita no
 * watchdog): a varredura alimenta o watchdog a cada requisição.
 * Preenche s->estado, s->mapa e s->sondas; não mexe em magic/versão/CRC. */
void varredura_executar(telem_suporte_t *s, uint32_t intervalo_ms, uint32_t timeout_ms);

/* Firmware demo: mapa dos PIDs que o Módulo A pede (pids.h), estado DEMO */
void varredura_demo(telem_suporte_t *s);
