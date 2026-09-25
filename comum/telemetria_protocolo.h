/**
 * telemetria_protocolo.h — Contrato de dados entre o Módulo A (aquisição,
 * ESP32-C3 no barramento OBD2) e o Módulo B (display, ESP32-S3), transportado
 * por ESP-NOW. Telemetria vai de A para B; comandos de configuração
 * (telem_comando_t, no fim do arquivo) voltam de B para A.
 *
 * Regras do contrato:
 *  - Só inteiros no ar. Nenhum float na struct: a precisão de cada grandeza
 *    fica explícita na escala documentada campo a campo. Isso elimina qualquer
 *    ambiguidade de representação binária entre os dois lados.
 *  - Struct empacotada (__attribute__((packed))): o layout no ar é exatamente
 *    a sequência de bytes declarada, sem padding dependente de compilador.
 *  - Magic number + versão no início, CRC-16 no fim. O receptor descarta
 *    silenciosamente qualquer pacote que falhe em qualquer uma das três
 *    verificações.
 *  - Os dois ESP32 são little-endian; os campos multi-byte vão no ar em
 *    little-endian, sem conversão.
 *
 * Qualquer mudança de campo exige incrementar TELEM_VERSAO e atualizar o
 * _Static_assert de tamanho no fim deste arquivo.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* "TELA" em little-endian — identifica o pacote de telemetria (A -> B) */
#define TELEM_MAGIC   0x414C4554u
/* "TCMD" em little-endian — identifica o pacote de comando (B -> A) */
#define TELEM_MAGIC_CMD 0x444D4354u
/* v2: flags b3/b4 e pacote de comando B -> A */
#define TELEM_VERSAO  2

/* Canal Wi-Fi fixo do enlace ESP-NOW. Os dois módulos precisam concordar,
 * já que nenhum deles se associa a um AP (sem DHCP, sem varredura). */
#define TELEM_CANAL_WIFI  1

/* Bits do campo flags */
#define TELEM_FLAG_MOTOR_LIGADO   (1u << 0) /* RPM válido e acima da marcha lenta mínima */
#define TELEM_FLAG_DADOS_VALIDOS  (1u << 1) /* PIDs essenciais respondendo dentro do timeout */
#define TELEM_FLAG_ALARME_ATIVO   (1u << 2) /* reservado: o Módulo A envia 0; o alarme é
                                             * decidido no Módulo B (limiar fica na NVS de lá) */
#define TELEM_FLAG_CAN_250K       (1u << 3) /* CAN do Módulo A a 250 kbit/s (0 = 500 kbit/s) */
#define TELEM_FLAG_BANCADA        (1u << 4) /* Módulo A gravado com o firmware de bancada */

typedef struct __attribute__((packed)) {
    uint32_t magic;          /* TELEM_MAGIC */
    uint8_t  versao;         /* TELEM_VERSAO */
    uint8_t  flags;          /* b0 motor ligado, b1 dados válidos, b2 alarme ativo */
    uint16_t rpm;            /* rpm */
    uint8_t  velocidade;     /* km/h */
    int16_t  temp_arref_d;   /* décimos de °C (905 = 90,5 °C) */
    int16_t  iat_d;          /* décimos de °C */
    uint16_t map_kpa;        /* kPa */
    uint16_t consumo_kml_c;  /* km/L × 100 (0 = parado ou inválido: usar L/h) */
    uint16_t consumo_lh_c;   /* L/h × 100 */
    uint8_t  etanol_pct;     /* % de etanol na mistura */
    uint8_t  nivel_pct;      /* % do tanque */
    uint16_t litros_d;       /* décimos de litro no tanque */
    uint16_t tensao_c;       /* centésimos de V (1385 = 13,85 V) */
    uint32_t uptime_ms;      /* ms desde o boot do Módulo A */
    uint16_t seq;            /* número de sequência incremental (detecção de perda) */
    uint16_t crc16;          /* CRC-16/CCITT-FALSE de todos os bytes anteriores */
} telem_pacote_t;

/* Quebra a compilação se algum campo mudar de tamanho sem querer.
 * 33 bytes = soma exata dos campos acima, sem padding (struct packed). */
#define TELEM_TAMANHO_PACOTE  33u
_Static_assert(sizeof(telem_pacote_t) == TELEM_TAMANHO_PACOTE,
               "telem_pacote_t mudou de tamanho — incremente TELEM_VERSAO e revise os dois modulos");

/**
 * CRC-16/CCITT-FALSE: polinômio 0x1021, valor inicial 0xFFFF, sem reflexão,
 * sem XOR final. Escolhido por ser trivial de conferir com calculadoras
 * online e por detectar bem erros em quadros curtos como este (33 bytes).
 * Implementado inline no header para os dois módulos usarem exatamente o
 * mesmo código — divergência de CRC entre os lados é indetectável em campo.
 */
static inline uint16_t telem_crc16(const uint8_t *dados, size_t tamanho)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < tamanho; i++) {
        crc ^= (uint16_t)dados[i] << 8;
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

/* CRC calculado sobre o pacote inteiro menos os 2 bytes finais (o próprio crc16) */
static inline uint16_t telem_crc16_pacote(const telem_pacote_t *p)
{
    return telem_crc16((const uint8_t *)p, sizeof(*p) - sizeof(p->crc16));
}

/* Validação completa de um pacote já copiado para memória alinhada */
static inline bool telem_pacote_valido(const telem_pacote_t *p)
{
    return p->magic == TELEM_MAGIC &&
           p->versao == TELEM_VERSAO &&
           telem_crc16_pacote(p) == p->crc16;
}

/* ---------------------------------------------------------------------------
 * Canal de retorno B -> A: comandos de configuração disparados pela tela.
 * Mesmo canal Wi-Fi, mesmas regras (packed, só inteiros, magic/versão/CRC).
 * Tamanho diferente do pacote de telemetria, então cada lado distingue o que
 * recebeu já pelo tamanho, antes de olhar o magic.
 * ------------------------------------------------------------------------- */
typedef enum {
    TELEM_CMD_DEFINIR_CAN_KBPS = 1,  /* valor = taxa do CAN em kbit/s (250 ou 500) */
} telem_comando_id_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;          /* TELEM_MAGIC_CMD */
    uint8_t  versao;         /* TELEM_VERSAO */
    uint8_t  comando;        /* telem_comando_id_t */
    uint16_t valor;          /* argumento; unidade definida por comando */
    uint16_t seq;            /* incremental no Módulo B */
    uint16_t crc16;          /* CRC-16/CCITT-FALSE dos bytes anteriores */
} telem_comando_t;

#define TELEM_TAMANHO_COMANDO  12u
_Static_assert(sizeof(telem_comando_t) == TELEM_TAMANHO_COMANDO,
               "telem_comando_t mudou de tamanho — incremente TELEM_VERSAO e revise os dois modulos");

static inline uint16_t telem_crc16_comando(const telem_comando_t *c)
{
    return telem_crc16((const uint8_t *)c, sizeof(*c) - sizeof(c->crc16));
}

static inline bool telem_comando_valido(const telem_comando_t *c)
{
    return c->magic == TELEM_MAGIC_CMD &&
           c->versao == TELEM_VERSAO &&
           telem_crc16_comando(c) == c->crc16;
}
