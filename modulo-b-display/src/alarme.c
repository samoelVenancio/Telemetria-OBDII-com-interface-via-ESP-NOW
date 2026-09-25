/**
 * alarme.c — Detecção de superaquecimento com média móvel e histerese.
 *
 * Por que média móvel: o sensor de temperatura tem ruído de quantização
 * (1 °C por LSB no PID 0x05) e o barramento pode entregar leitura espúria;
 * uma amostra isolada não pode disparar alarme dentro do carro andando.
 *
 * Por que histerese: sem ela, com a média oscilando em torno do limiar o
 * alarme piscaria (liga/desliga a cada amostra). Regra:
 *   ativa   quando média >= limiar
 *   desativa quando média <= limiar − histerese
 * No meio, mantém o estado anterior.
 *
 * Nada de LVGL aqui: o módulo devolve um enum; quem pinta a tela é a UI.
 */
#include <string.h>

#include "alarme.h"
#include "nvm_config.h"

/* Janela máxima — casa com NVM_JANELA_MAX; o tamanho efetivo vem da NVS */
#define JANELA_MAXIMA  32

static int16_t s_amostras[JANELA_MAXIMA];
static uint8_t s_quantidade = 0;   /* amostras válidas acumuladas (até a janela) */
static uint8_t s_indice = 0;       /* posição circular de escrita */
static uint8_t s_janela = 8;
static int16_t s_limiar_d = 1050;
static int16_t s_histerese_d = 30;
static alarme_estado_t s_estado = ALARME_SEM_DADOS;
static int16_t s_media_d = 0;

void alarme_iniciar(void)
{
    alarme_reconfigurar();
}

void alarme_reconfigurar(void)
{
    const nvm_config_t *cfg = nvm_config();
    s_limiar_d = cfg->limiar_temp_d;
    s_histerese_d = cfg->histerese_d;
    s_janela = cfg->janela_media;
    if (s_janela < NVM_JANELA_MIN) s_janela = NVM_JANELA_MIN;
    if (s_janela > JANELA_MAXIMA) s_janela = JANELA_MAXIMA;

    /* Janela zerada: mudar parâmetro no meio de uma média misturaria regimes */
    memset(s_amostras, 0, sizeof(s_amostras));
    s_quantidade = 0;
    s_indice = 0;
    s_estado = ALARME_SEM_DADOS;
    s_media_d = 0;
}

alarme_estado_t alarme_processar(int16_t temp_d, bool amostra_valida)
{
    if (!amostra_valida) {
        /* Não zera a janela por uma falha pontual, mas se a fonte sumir de
         * vez o estado degrada para SEM_DADOS em vez de congelar um valor. */
        s_quantidade = (s_quantidade > 0) ? (uint8_t)(s_quantidade - 1) : 0;
        if (s_quantidade == 0) {
            s_estado = ALARME_SEM_DADOS;
            s_media_d = 0;
        }
        return s_estado;
    }

    s_amostras[s_indice] = temp_d;
    s_indice = (uint8_t)((s_indice + 1) % s_janela);
    if (s_quantidade < s_janela) {
        s_quantidade++;
    }

    int32_t soma = 0;
    for (uint8_t i = 0; i < s_quantidade; i++) {
        soma += s_amostras[i];
    }
    s_media_d = (int16_t)(soma / s_quantidade);

    switch (s_estado) {
    case ALARME_ATIVO:
        if (s_media_d <= (int16_t)(s_limiar_d - s_histerese_d)) {
            s_estado = ALARME_NORMAL;
        }
        break;
    case ALARME_NORMAL:
    case ALARME_SEM_DADOS:
    default:
        if (s_media_d >= s_limiar_d) {
            s_estado = ALARME_ATIVO;
        } else {
            s_estado = ALARME_NORMAL;
        }
        break;
    }
    return s_estado;
}

alarme_estado_t alarme_estado(void)
{
    return s_estado;
}

int16_t alarme_media_d(void)
{
    return s_media_d;
}
