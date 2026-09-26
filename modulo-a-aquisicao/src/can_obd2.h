/**
 * can_obd2.h — Acesso ao barramento CAN do veículo via TWAI e diálogo
 * OBD2 (ISO 15765-4, 11 bits) com o ECM. Taxa configurável: 500 kbit/s no
 * Fiesta, 250 kbit/s no simulador de bancada.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#define CAN_KBPS_CARRO    500u  /* taxa medida no New Fiesta (padrão de fábrica) */
#define CAN_KBPS_BANCADA  250u  /* taxa do simulador Arduino+MCP2515 */

/* Resultado de uma janela de escuta em listen-only */
typedef struct {
    uint32_t quadros_validos;   /* quadros completos recebidos */
    uint32_t erros_barramento;  /* erros de bit/stuff/forma/CRC/ACK vistos */
} can_obd2_escuta_t;

/* Taxa gravada na NVS (CAN_KBPS_CARRO se vazia ou inválida) */
uint16_t can_obd2_taxa_salva(void);

/* Grava a taxa na NVS. Só 250 e 500 são aceitas. Passa a valer no próximo boot. */
esp_err_t can_obd2_salvar_taxa(uint16_t kbps);

/* Taxa com que o driver está instalado agora */
uint16_t can_obd2_taxa_atual(void);

/* Instala o driver TWAI em TWAI_MODE_LISTEN_ONLY na taxa pedida (não gera
 * ACK, não gera quadro de erro, não transmite). Se já houver driver
 * instalado, reinstala. */
esp_err_t can_obd2_iniciar_escuta(uint16_t kbps);

/* Escuta pela janela dada e conta quadros válidos e erros de barramento */
can_obd2_escuta_t can_obd2_escutar(uint32_t janela_ms);

/* SÓ BANCADA. Instala o driver em modo NORMAL na taxa pedida, sem filtro, e
 * conta quadros válidos na janela. Em modo normal nós damos ACK: na taxa
 * certa os quadros do simulador fecham; na errada só aparecem erros. Nunca
 * usar no carro — transmitir numa taxa não provada gera quadros de erro no
 * barramento do veículo. */
can_obd2_escuta_t can_obd2_sondar_normal(uint16_t kbps, uint32_t janela_ms);

/* Reinstala o driver em TWAI_MODE_NORMAL, na mesma taxa da escuta, com
 * filtro de hardware para 0x7E8. Só chamar depois de confirmar tráfego. */
esp_err_t can_obd2_modo_normal(void);

/* Quantas vezes seguidas o controlador caiu em bus-off sem nenhuma resposta
 * válida no meio. Zera a cada resposta boa do ECM. */
uint32_t can_obd2_bus_off_seguidos(void);

/* Requisita um PID do Modo 01 (single frame) e espera a resposta do ECM.
 * Em sucesso, copia os bytes de dados (A, B, ...) para 'resposta' e escreve a
 * quantidade em 'tamanho' (máx. 5 num single frame). Retorna ESP_ERR_TIMEOUT
 * se o ECM não responder dentro de 'timeout_ms'. */
esp_err_t can_obd2_requisitar_pid(uint8_t pid, uint8_t *resposta,
                                  size_t *tamanho, uint32_t timeout_ms);

/* Timestamp (ms desde o boot) do último quadro visto no barramento —
 * insumo da gestão de energia (energia.c). */
int64_t can_obd2_ultima_atividade_ms(void);

/* true se o ECM já respondeu algum PID desde o boot (carro ligado) */
bool can_obd2_ecm_respondeu(void);

/* Desinstala o driver e trava o pino TX em recessivo (nível alto) durante o
 * deep sleep. Obrigatório antes de dormir — ver comentário no .c. */
void can_obd2_preparar_sono(void);

/* FASE 2 — STUB. Leitura de DTCs (Modo 03). Ver comentário no .c. */
esp_err_t can_obd2_ler_dtcs(uint16_t *codigos, size_t maximo, size_t *quantidade);
