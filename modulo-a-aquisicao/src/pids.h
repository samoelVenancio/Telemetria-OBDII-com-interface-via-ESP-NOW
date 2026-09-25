/**
 * pids.h — Tabela de PIDs do veículo, agendamento de leitura e decodificação
 * J1979 para as grandezas internas.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* PIDs Modo 01 confirmados por varredura real no New Fiesta 1.6 flex 2014 */
#define PID_TEMP_ARREF  0x05u  /* A-40 °C */
#define PID_MAP         0x0Bu  /* A kPa */
#define PID_RPM         0x0Cu  /* ((256A)+B)/4 rpm */
#define PID_VELOCIDADE  0x0Du  /* A km/h */
#define PID_IAT         0x0Fu  /* A-40 °C */
#define PID_NIVEL       0x2Fu  /* (100/255)A % */
#define PID_TENSAO      0x42u  /* ((256A)+B)/1000 V */
#define PID_ETANOL      0x52u  /* (100/255)A % */

/* Estado consolidado do veículo, em inteiros escalados iguais aos do pacote.
 * Cada grandeza carrega seu próprio flag de validade: um PID que estourou o
 * timeout fica inválido sem contaminar os demais. */
typedef struct {
    uint16_t rpm;            bool rpm_valido;
    uint8_t  velocidade_kmh; bool velocidade_valida;
    int16_t  temp_arref_d;   bool temp_arref_valida;   /* décimos de °C */
    int16_t  iat_d;          bool iat_valida;          /* décimos de °C */
    uint16_t map_kpa;        bool map_valido;
    uint8_t  etanol_pct;     bool etanol_valido;
    uint8_t  nivel_pct;      bool nivel_valido;
    uint16_t tensao_c;       bool tensao_valida;       /* centésimos de V */
} dados_veiculo_t;

/* Zera a tabela de agendamento (todos os PIDs devidos imediatamente) */
void pids_iniciar(void);

/* Índice do PID mais atrasado que já venceu, ou -1 se nenhum venceu ainda */
int pids_proximo_devido(int64_t agora_ms);

/* Código J1979 do PID no índice dado */
uint8_t pids_codigo(int indice);

/* Reagenda o PID do índice para daqui a um período (chamado após a tentativa,
 * com ou sem sucesso — falha não pode acelerar o ciclo) */
void pids_reagendar(int indice, int64_t agora_ms);

/* Decodifica a resposta de um PID (bytes A, B, ...) para dentro de 'v'
 * aplicando a fórmula J1979. Retorna false se faltarem bytes. */
bool pids_decodificar(uint8_t pid, const uint8_t *dados, size_t n, dados_veiculo_t *v);

/* Marca a grandeza do PID como inválida (timeout/erro) sem tocar nas outras */
void pids_marcar_invalido(uint8_t pid, dados_veiculo_t *v);
