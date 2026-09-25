/**
 * alarme.h — Alarme de temperatura do arrefecimento: média móvel + histerese.
 * Toda a lógica mora aqui; a UI só consome o estado resultante.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ALARME_SEM_DADOS = 0,  /* sem amostra válida recente — não dá para opinar */
    ALARME_NORMAL,
    ALARME_ATIVO,
} alarme_estado_t;

/* Lê limiar/histerese/janela da nvm_config e zera a janela */
void alarme_iniciar(void);

/* Relê a nvm_config (chamar após salvar novos parâmetros na tela de alarme) */
void alarme_reconfigurar(void);

/* Alimenta uma amostra (décimos de °C) e devolve o estado atualizado.
 * amostra_valida=false não entra na média; janela velha demais vira SEM_DADOS. */
alarme_estado_t alarme_processar(int16_t temp_d, bool amostra_valida);

/* Último estado calculado, sem alimentar amostra */
alarme_estado_t alarme_estado(void);

/* Média móvel corrente em décimos de °C (0 se sem dados) */
int16_t alarme_media_d(void);
