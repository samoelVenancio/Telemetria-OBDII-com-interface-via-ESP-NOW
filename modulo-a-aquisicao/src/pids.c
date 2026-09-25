/**
 * pids.c — Agendamento e decodificação dos PIDs OBD2.
 *
 * Por que existe uma tabela de períodos:
 *  - A cadência entre requisições é espaçada (padrão 100 ms — ver main.c),
 *    o que dá um orçamento de ~10 requisições/s. A soma das frequências da
 *    tabela abaixo PRECISA caber nesse orçamento:
 *        3 PIDs rápidos a 400 ms  -> 7,5 req/s
 *        3 PIDs médios a 2 s      -> 1,5 req/s
 *        2 PIDs lentos a 10 s     -> 0,2 req/s
 *        total                    -> 9,2 req/s  (cabe, com folga p/ retries)
 *  - Se estourar o orçamento, o escalonador não quebra: ele sempre atende o
 *    PID mais atrasado primeiro e o ciclo inteiro apenas se alonga.
 *
 * Nota do projeto: 0x10 (MAF) e 0x5E (vazão de combustível) foram confirmados
 * AUSENTES neste veículo (motor speed-density). O consumo é CALCULADO em
 * consumo.c a partir de RPM/MAP/IAT — essa é a contribuição do projeto, não
 * uma limitação a contornar.
 */
#include "pids.h"

/* Períodos de releitura por classe de PID */
#define PERIODO_RAPIDO_MS  400u    /* RPM, velocidade, MAP: mudam a cada instante */
#define PERIODO_MEDIO_MS   2000u   /* IAT, temp. arrefecimento, tensão: inércia térmica/elétrica */
#define PERIODO_LENTO_MS   10000u  /* etanol e nível do tanque: só mudam abastecendo */

typedef struct {
    uint8_t  pid;
    uint32_t periodo_ms;
    int64_t  devido_em_ms; /* próximo instante de leitura (0 = imediato) */
} agenda_pid_t;

static agenda_pid_t s_agenda[] = {
    { PID_RPM,        PERIODO_RAPIDO_MS, 0 },
    { PID_VELOCIDADE, PERIODO_RAPIDO_MS, 0 },
    { PID_MAP,        PERIODO_RAPIDO_MS, 0 },
    { PID_IAT,        PERIODO_MEDIO_MS,  0 },
    { PID_TEMP_ARREF, PERIODO_MEDIO_MS,  0 },
    { PID_TENSAO,     PERIODO_MEDIO_MS,  0 },
    { PID_ETANOL,     PERIODO_LENTO_MS,  0 },
    { PID_NIVEL,      PERIODO_LENTO_MS,  0 },
};
#define TOTAL_PIDS ((int)(sizeof(s_agenda) / sizeof(s_agenda[0])))

void pids_iniciar(void)
{
    for (int i = 0; i < TOTAL_PIDS; i++) {
        s_agenda[i].devido_em_ms = 0;
    }
}

int pids_proximo_devido(int64_t agora_ms)
{
    int escolhido = -1;
    int64_t mais_atrasado = 0;
    for (int i = 0; i < TOTAL_PIDS; i++) {
        if (s_agenda[i].devido_em_ms > agora_ms) {
            continue; /* ainda não venceu */
        }
        int64_t atraso = agora_ms - s_agenda[i].devido_em_ms;
        if (escolhido < 0 || atraso > mais_atrasado) {
            escolhido = i;
            mais_atrasado = atraso;
        }
    }
    return escolhido;
}

uint8_t pids_codigo(int indice)
{
    return s_agenda[indice].pid;
}

void pids_reagendar(int indice, int64_t agora_ms)
{
    s_agenda[indice].devido_em_ms = agora_ms + s_agenda[indice].periodo_ms;
}

bool pids_decodificar(uint8_t pid, const uint8_t *d, size_t n, dados_veiculo_t *v)
{
    switch (pid) {
    case PID_RPM: /* ((256A)+B)/4 rpm */
        if (n < 2) return false;
        v->rpm = (uint16_t)((((uint32_t)d[0] << 8) | d[1]) / 4u);
        v->rpm_valido = true;
        return true;

    case PID_VELOCIDADE: /* A km/h */
        if (n < 1) return false;
        v->velocidade_kmh = d[0];
        v->velocidade_valida = true;
        return true;

    case PID_TEMP_ARREF: /* A-40 °C -> décimos */
        if (n < 1) return false;
        v->temp_arref_d = (int16_t)(((int16_t)d[0] - 40) * 10);
        v->temp_arref_valida = true;
        return true;

    case PID_IAT: /* A-40 °C -> décimos */
        if (n < 1) return false;
        v->iat_d = (int16_t)(((int16_t)d[0] - 40) * 10);
        v->iat_valida = true;
        return true;

    case PID_MAP: /* A kPa */
        if (n < 1) return false;
        v->map_kpa = d[0];
        v->map_valido = true;
        return true;

    case PID_TENSAO: /* ((256A)+B)/1000 V -> centésimos: mV/10 */
        if (n < 2) return false;
        v->tensao_c = (uint16_t)(((((uint32_t)d[0] << 8) | d[1])) / 10u);
        v->tensao_valida = true;
        return true;

    case PID_ETANOL: /* (100/255)A %, com arredondamento */
        if (n < 1) return false;
        v->etanol_pct = (uint8_t)(((uint32_t)d[0] * 100u + 127u) / 255u);
        v->etanol_valido = true;
        return true;

    case PID_NIVEL: /* (100/255)A %, com arredondamento */
        if (n < 1) return false;
        v->nivel_pct = (uint8_t)(((uint32_t)d[0] * 100u + 127u) / 255u);
        v->nivel_valido = true;
        return true;

    default:
        return false;
    }
}

void pids_marcar_invalido(uint8_t pid, dados_veiculo_t *v)
{
    switch (pid) {
    case PID_RPM:        v->rpm_valido = false;        break;
    case PID_VELOCIDADE: v->velocidade_valida = false; break;
    case PID_TEMP_ARREF: v->temp_arref_valida = false; break;
    case PID_IAT:        v->iat_valida = false;        break;
    case PID_MAP:        v->map_valido = false;        break;
    case PID_TENSAO:     v->tensao_valida = false;     break;
    case PID_ETANOL:     v->etanol_valido = false;     break;
    case PID_NIVEL:      v->nivel_valido = false;      break;
    default: break;
    }
}
