/**
 * demo.h — Dados de veículo SIMULADOS para testar o Módulo B sem carro e sem
 * o simulador Arduino (env modulo_a_demo do platformio.ini).
 *
 * Nesse modo o driver TWAI nunca é instalado: o Módulo A não escuta nem
 * transmite nada no CAN, só gera os dados e manda pelo ESP-NOW.
 */
#pragma once

#include <stdint.h>
#include "pids.h"

/* Preenche 'd' com o estado simulado no instante 't_ms' desde o boot.
 * Ciclo de condução de 90 s (marcha lenta, aceleração com trocas, cruzeiro a
 * 80 km/h, frenagem), aquecimento do motor nos primeiros 3 min e, depois
 * disso, um episódio de superaquecimento (até 108 °C) a cada 5 min. */
void demo_gerar(int64_t t_ms, dados_veiculo_t *d);
