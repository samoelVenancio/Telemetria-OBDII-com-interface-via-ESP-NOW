/**
 * main.c — Módulo A (aquisição) — ESP32-C3 SuperMini no OBD2 do New Fiesta.
 *
 * Orquestração:
 *  1. Boot em TWAI somente-escuta por 3 s na taxa gravada na NVS (ver
 *     confirmar_barramento()). Sem tráfego -> carro dormindo -> deep sleep
 *     direto, sem transmitir um único bit no barramento nem no ar.
 *  2. Com tráfego confirmado: TWAI em modo normal, ESP-NOW no ar e duas tasks:
 *       - tarefa_aquisicao: uma requisição OBD2 por vez, espaçadas (nunca
 *         rajada), timeout individual por PID, recálculo do consumo.
 *       - tarefa_envio: snapshot da telemetria a 10 Hz via ESP-NOW e
 *         atendimento dos comandos vindos do Módulo B.
 *  3. Ambas inscritas no watchdog de task; ambas com prioridade acima do idle.
 *
 * Troca de taxa pela tela: o Módulo B manda o comando, esta task grava na
 * NVS e REINICIA o módulo. Reiniciar (em vez de reinstalar o driver ao
 * vivo) é proposital: a taxa nova passa pela mesma escuta obrigatória do
 * boot, sem caminho alternativo para transmitir sem escutar antes.
 *
 * O ESP32-C3 é single-core: as duas tasks dividem o core 0. A separação em
 * tasks não é por paralelismo, é para a cadência de envio (10 Hz firmes, via
 * vTaskDelayUntil) não depender do tempo variável do ciclo OBD2.
 */
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "telemetria_protocolo.h"
#include "can_obd2.h"
#include "pids.h"
#include "consumo.h"
#include "enlace_espnow.h"
#include "energia.h"

static const char *TAG = "principal";

/* Definido pelo env do platformio.ini. Sem ele, vale o padrão SEGURO
 * (firmware do carro, escuta estrita). */
#ifndef CAN_BANCADA_ACEITA_SEM_ACK
#define CAN_BANCADA_ACEITA_SEM_ACK 0
#endif

/* Cadências e limites — todos com unidade no nome */
#define INTERVALO_ENTRE_REQ_MS    100u   /* espaçamento mínimo entre requisições OBD2 (configurável) */
#define TIMEOUT_RESPOSTA_PID_MS   200u   /* ECM calado além disso -> PID inválido, ciclo segue */
#define PERIODO_ENVIO_ESPNOW_MS   100u   /* 10 Hz */
#define JANELA_ESCUTA_BOOT_MS     3000u  /* escuta obrigatória antes de transmitir */
#define RPM_MINIMO_MOTOR_LIGADO   400u   /* abaixo disso é partida/bounce, não marcha lenta */
#define JANELA_SONDA_BANCADA_MS   500u   /* simulador manda fundo a cada 50 ms: ~10 quadros */
#define LIMITE_BUS_OFF_SEGUIDOS   3u     /* passou disso: reinicia e refaz a detecção do barramento */
#define PERIODO_LOG_RESUMO_MS     5000u  /* resumo no monitor serial */

/* Capacidade do tanque para converter nível % em litros.
 * TODO: confirmar no manual do New Fiesta hatch (valor de placa: 48 L). */
#define TANQUE_CAPACIDADE_DL      480u   /* décimos de litro */

/* Estado compartilhado entre as duas tasks — sempre acessado sob a trava.
 * Spinlock (portMUX) e não mutex: as seções críticas são cópias de structs
 * pequenas, mais curtas que um context switch. */
static dados_veiculo_t s_dados;
static consumo_saida_t s_consumo;
static portMUX_TYPE s_trava_dados = portMUX_INITIALIZER_UNLOCKED;

static void tarefa_aquisicao(void *arg)
{
    (void)arg;
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    for (;;) {
        esp_task_wdt_reset();
        int64_t agora = esp_timer_get_time() / 1000;

        int indice = pids_proximo_devido(agora);
        if (indice >= 0) {
            uint8_t pid = pids_codigo(indice);
            uint8_t resposta[8];
            size_t tamanho = 0;

            /* Trabalha numa cópia local e publica o resultado de uma vez:
             * a task de envio nunca vê estado meio-atualizado. */
            dados_veiculo_t novo;
            portENTER_CRITICAL(&s_trava_dados);
            novo = s_dados;
            portEXIT_CRITICAL(&s_trava_dados);

            esp_err_t r = can_obd2_requisitar_pid(pid, resposta, &tamanho,
                                                  TIMEOUT_RESPOSTA_PID_MS);
            if (r == ESP_OK && pids_decodificar(pid, resposta, tamanho, &novo)) {
                /* dado novo já está em 'novo' */
            } else {
                pids_marcar_invalido(pid, &novo);
                ESP_LOGD(TAG, "PID 0x%02X sem resposta valida (%s)",
                         pid, esp_err_to_name(r));
            }
            pids_reagendar(indice, agora);

            consumo_saida_t c = consumo_calcular(&novo);

            portENTER_CRITICAL(&s_trava_dados);
            s_dados = novo;
            s_consumo = c;
            portEXIT_CRITICAL(&s_trava_dados);
        }

        /* Bus-off repetido: a recuperação não resolve (taxa mudou, fiação,
         * simulador trocado de taxa). Reiniciar refaz a detecção do barramento
         * pelo mesmo caminho seguro do boot, em vez de insistir no escuro. */
        if (can_obd2_bus_off_seguidos() >= LIMITE_BUS_OFF_SEGUIDOS) {
            ESP_LOGE(TAG, "%u bus-off seguidos — reiniciando para refazer a deteccao",
                     (unsigned)LIMITE_BUS_OFF_SEGUIDOS);
            esp_restart(); /* não retorna */
        }

        /* ECM calado? (deep sleep lá dentro se sim — não retorna) */
        energia_avaliar(can_obd2_ultima_atividade_ms(), can_obd2_ecm_respondeu());

        /* Cadência espaçada: NUNCA rajada. Mesmo com vários PIDs vencidos,
         * sai exatamente uma requisição por intervalo. */
        vTaskDelay(pdMS_TO_TICKS(INTERVALO_ENTRE_REQ_MS));
    }
}

static void tarefa_envio(void *arg)
{
    (void)arg;
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    TickType_t proximo_disparo = xTaskGetTickCount();

    for (;;) {
        esp_task_wdt_reset();

        dados_veiculo_t d;
        consumo_saida_t c;
        portENTER_CRITICAL(&s_trava_dados);
        d = s_dados;
        c = s_consumo;
        portEXIT_CRITICAL(&s_trava_dados);

        /* Monta o pacote no contrato do header comum. Campo inválido vai
         * como 0; a validade agregada segue no flag b1. */
        telem_pacote_t p = { 0 };
        p.rpm          = d.rpm_valido ? d.rpm : 0;
        p.velocidade   = d.velocidade_valida ? d.velocidade_kmh : 0;
        p.temp_arref_d = d.temp_arref_valida ? d.temp_arref_d : 0;
        p.iat_d        = d.iat_valida ? d.iat_d : 0;
        p.map_kpa      = d.map_valido ? d.map_kpa : 0;
        p.etanol_pct   = d.etanol_valido ? d.etanol_pct : 0;
        p.nivel_pct    = d.nivel_valido ? d.nivel_pct : 0;
        p.tensao_c     = d.tensao_valida ? d.tensao_c : 0;
        p.litros_d     = d.nivel_valido
                         ? (uint16_t)((uint32_t)d.nivel_pct * TANQUE_CAPACIDADE_DL / 100u)
                         : 0;
        if (c.valido) {
            p.consumo_kml_c = c.kml_c;
            p.consumo_lh_c  = c.lh_c;
        }

        if (d.rpm_valido && d.rpm >= RPM_MINIMO_MOTOR_LIGADO) {
            p.flags |= TELEM_FLAG_MOTOR_LIGADO;
        }
        /* "Dados válidos" = os PIDs essenciais do painel estão respondendo */
        if (d.rpm_valido && d.velocidade_valida && d.map_valido && d.temp_arref_valida) {
            p.flags |= TELEM_FLAG_DADOS_VALIDOS;
        }
        /* b2 (alarme) fica 0: quem decide alarme é o Módulo B (limiar na NVS de lá) */
        if (can_obd2_taxa_atual() == CAN_KBPS_BANCADA) {
            p.flags |= TELEM_FLAG_CAN_250K;
        }
        if (CAN_BANCADA_ACEITA_SEM_ACK) {
            p.flags |= TELEM_FLAG_BANCADA;
        }

        p.uptime_ms = (uint32_t)(esp_timer_get_time() / 1000);

        enlace_espnow_enviar(&p); /* completa magic/versão/seq/CRC e transmite */

        /* Resumo periódico no serial: prova de vida do ciclo inteiro */
        static int64_t ultimo_log_ms = 0;
        int64_t agora_ms = esp_timer_get_time() / 1000;
        if (agora_ms - ultimo_log_ms >= PERIODO_LOG_RESUMO_MS) {
            ultimo_log_ms = agora_ms;
            ESP_LOGI(TAG, "CAN %u kbit/s | rpm %u vel %u temp %d.%d | dados %s | "
                          "ESP-NOW seq %u falhas %lu",
                     can_obd2_taxa_atual(), p.rpm, p.velocidade,
                     p.temp_arref_d / 10, abs(p.temp_arref_d % 10),
                     (p.flags & TELEM_FLAG_DADOS_VALIDOS) ? "validos" : "INVALIDOS",
                     p.seq, (unsigned long)enlace_espnow_falhas_envio());
        }

        /* Pedido de troca de taxa vindo da tela do Módulo B */
        uint16_t kbps_pedida = enlace_espnow_consumir_taxa_pedida();
        if (kbps_pedida != 0 && kbps_pedida != can_obd2_taxa_atual()) {
            if (can_obd2_salvar_taxa(kbps_pedida) == ESP_OK) {
                ESP_LOGW(TAG, "taxa CAN trocada para %u kbit/s pela tela — reiniciando "
                              "para refazer a escuta obrigatoria", kbps_pedida);
                esp_restart(); /* não retorna */
            }
        }

        /* vTaskDelayUntil, não vTaskDelay: mantém 10 Hz absolutos mesmo que
         * a montagem/envio consuma tempo variável. */
        vTaskDelayUntil(&proximo_disparo, pdMS_TO_TICKS(PERIODO_ENVIO_ESPNOW_MS));
    }
}

/**
 * Regra de ouro do boot: escutar antes de falar. Retorna true se o
 * barramento está vivo e o driver pode ir para o modo normal.
 *
 *  - Quadro válido na taxa da NVS: barramento vivo na taxa certa. Segue.
 *  - Silêncio total: carro dormindo. Não segue.
 *  - Só erros de barramento: tem sinal, mas nenhum quadro fecha. Duas causas:
 *      (a) taxa errada (ex.: NVS em 250k e módulo plugado no carro a 500k);
 *      (b) bancada com 2 nós, onde nós em listen-only não damos ACK e
 *          todo quadro do simulador morre no delimitador de ACK.
 *    Firmware do carro: testa a OUTRA taxa, ainda em listen-only (inofensivo),
 *    e só adota se aparecer quadro válido. Nunca transmite numa taxa que não
 *    foi provada — erro de taxa no carro geraria quadros de erro no
 *    barramento do Fiesta.
 *    Firmware de bancada: aceita (b), porque sem um 3º nó não há como provar
 *    a taxa por escuta. Por isso esse relaxamento só existe no env de bancada.
 */
static bool confirmar_barramento(void)
{
    uint16_t kbps = can_obd2_taxa_salva();
    ESP_ERROR_CHECK(can_obd2_iniciar_escuta(kbps));
    can_obd2_escuta_t e = can_obd2_escutar(JANELA_ESCUTA_BOOT_MS);

    if (e.quadros_validos > 0) {
        return true;
    }
    if (e.erros_barramento == 0) {
        ESP_LOGW(TAG, "barramento em silencio a %u kbit/s", kbps);
        energia_rede_em_repouso();
        return false;
    }

    uint16_t outra = (kbps == CAN_KBPS_CARRO) ? CAN_KBPS_BANCADA : CAN_KBPS_CARRO;

#if CAN_BANCADA_ACEITA_SEM_ACK
    /* Bancada: a escuta não prova a taxa (sem 3º nó ninguém dá ACK), então
     * sonda em modo NORMAL, onde nós mesmos damos o ACK. Na taxa certa os
     * quadros de fundo do simulador passam a fechar; na errada, só erros.
     * Transmitir numa taxa errada aqui é inofensivo — só existe o simulador. */
    ESP_LOGW(TAG, "BANCADA: %lu erro(s) sem quadro valido em escuta — sondando as taxas",
             (unsigned long)e.erros_barramento);
    const uint16_t candidatas[] = { kbps, outra };
    for (size_t i = 0; i < sizeof(candidatas) / sizeof(candidatas[0]); i++) {
        e = can_obd2_sondar_normal(candidatas[i], JANELA_SONDA_BANCADA_MS);
        if (e.quadros_validos > 0) {
            if (candidatas[i] != kbps) {
                can_obd2_salvar_taxa(candidatas[i]);
            }
            ESP_LOGI(TAG, "BANCADA: simulador respondendo a %u kbit/s", candidatas[i]);
            return true;
        }
    }
    ESP_LOGW(TAG, "BANCADA: nenhuma taxa fechou quadro — simulador desligado ou fiacao?");
    return false;
#else
    ESP_LOGW(TAG, "so erros a %u kbit/s — taxa errada? testando %u kbit/s em escuta",
             kbps, outra);
    ESP_ERROR_CHECK(can_obd2_iniciar_escuta(outra));
    e = can_obd2_escutar(JANELA_ESCUTA_BOOT_MS);
    if (e.quadros_validos > 0) {
        can_obd2_salvar_taxa(outra); /* autodetectada: vira a nova taxa padrão */
        return true;
    }
    return false;
#endif
}

void app_main(void)
{
    /* NVS primeiro: consumo.c (VE) e o Wi-Fi dependem dela */
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        r = nvs_flash_init();
    }
    ESP_ERROR_CHECK(r);

    energia_iniciar();
    consumo_iniciar();
    pids_iniciar();

    ESP_LOGI(TAG, "firmware de %s", CAN_BANCADA_ACEITA_SEM_ACK ? "BANCADA" : "CARRO");

    if (!confirmar_barramento()) {
        ESP_LOGW(TAG, "sem trafego CAN utilizavel — carro dormindo; "
                      "nada sera transmitido");
        energia_dormir_agora(); /* não retorna; timer reacorda (60 s ou 5 min) */
    }

    ESP_ERROR_CHECK(can_obd2_modo_normal());
    ESP_ERROR_CHECK(enlace_espnow_iniciar());

    xTaskCreate(tarefa_aquisicao, "aquisicao", 4096, NULL, 5, NULL);
    xTaskCreate(tarefa_envio,     "envio",     4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "modulo A no ar: CAN %u kbit/s, OBD2 a cada %u ms, ESP-NOW a %u Hz",
             can_obd2_taxa_atual(), (unsigned)INTERVALO_ENTRE_REQ_MS,
             (unsigned)(1000u / PERIODO_ENVIO_ESPNOW_MS));
}
