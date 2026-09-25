/**
 * can_obd2.c — Barramento CAN via driver TWAI nativo do ESP32-C3.
 *
 * O que este arquivo faz e por quê:
 *  - Boot em TWAI_MODE_LISTEN_ONLY: o controlador não gera ACK nem quadros de
 *    erro, então é eletricamente invisível para o carro. Só depois de
 *    confirmar tráfego o driver é reinstalado em modo normal.
 *  - A escuta separa QUADROS VÁLIDOS de ERROS DE BARRAMENTO, porque os dois
 *    contam histórias diferentes:
 *      quadros válidos  -> barramento vivo E na nossa taxa. Prova forte.
 *      só erros         -> tem sinal elétrico, mas (a) a taxa está errada ou
 *                          (b) é uma bancada com 2 nós: em listen-only nós
 *                          não damos ACK, o transmissor sinaliza erro de ACK
 *                          no delimitador e TODO quadro chega corrompido.
 *    Quem decide o que fazer com cada caso é o main.c.
 *  - Requisições OBD2 Modo 01 em ISO-TP single frame. Multi-frame com flow
 *    control fica para a fase 2 (Modo 03).
 *  - Filtro de aceitação: aberto na escuta (qualquer tráfego interessa);
 *    no modo normal, filtro único de hardware para 0x7E8.
 *  - A taxa (250/500 kbit/s) vive na NVS e é trocada pela tela do Módulo B.
 *
 * Levantamento real do veículo (varredura de PIDs):
 *  - 500 kbit/s, requisições para 0x7E0, ECM responde em 0x7E8.
 *  - J1962: CAN-H pino 6, CAN-L pino 14, GND 4/5, 12V permanente no 16.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"

#include "can_obd2.h"

static const char *TAG = "can_obd2";

/* Pinos do SN65HVD230 no ESP32-C3 SuperMini (montagem conferida).
 * Nenhum dos dois é pino de strapping do C3 (2, 8 e 9). */
#define CAN_GPIO_TX  6   /* ESP32 TX -> CTX do SN65HVD230 */
#define CAN_GPIO_RX  7   /* ESP32 RX <- CRX do SN65HVD230 */

#define OBD2_ID_REQUISICAO   0x7E0u  /* requisição física ao ECM */
#define OBD2_ID_RESPOSTA     0x7E8u  /* resposta do ECM */
#define OBD2_MODO_DADOS      0x01u   /* Modo 01 — dados atuais */
#define OBD2_TIMEOUT_TX_MS   50u     /* espaço na fila de TX; barramento saudável esvazia em <1 ms */
#define TWAI_FILA_RX_QUADROS 32u

/* ISO 15765-4 exige DLC 8; o valor de padding não é normativo.
 * TODO: se o ECM parar de responder após mudanças, testar padding 0x00. */
#define OBD2_PADDING         0x55u

#define NVS_NAMESPACE        "telemetria"
#define NVS_CHAVE_CAN_KBPS   "can_kbps"

static int64_t s_ultima_atividade_ms = 0;
static uint16_t s_kbps = CAN_KBPS_CARRO;
static bool s_instalado = false;

static int64_t agora_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static bool taxa_suportada(uint16_t kbps)
{
    return kbps == CAN_KBPS_CARRO || kbps == CAN_KBPS_BANCADA;
}

uint16_t can_obd2_taxa_salva(void)
{
    uint16_t kbps = CAN_KBPS_CARRO;
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u16(h, NVS_CHAVE_CAN_KBPS, &kbps);
        nvs_close(h);
    }
    return taxa_suportada(kbps) ? kbps : CAN_KBPS_CARRO;
}

esp_err_t can_obd2_salvar_taxa(uint16_t kbps)
{
    if (!taxa_suportada(kbps)) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (r != ESP_OK) {
        return r;
    }
    r = nvs_set_u16(h, NVS_CHAVE_CAN_KBPS, kbps);
    if (r == ESP_OK) {
        r = nvs_commit(h);
    }
    nvs_close(h);
    ESP_LOGI(TAG, "taxa CAN gravada na NVS: %u kbit/s", kbps);
    return r;
}

uint16_t can_obd2_taxa_atual(void)
{
    return s_kbps;
}

static void desinstalar(void)
{
    if (s_instalado) {
        twai_stop();
        twai_driver_uninstall();
        s_instalado = false;
    }
}

/* Instala e inicia o driver no modo pedido. O modo do TWAI é fixado na
 * instalação, então trocar de listen-only para normal (ou de taxa) exige
 * stop + uninstall + install. */
static esp_err_t instalar_driver(twai_mode_t modo, uint16_t kbps, bool filtrar_resposta_ecm)
{
    desinstalar();

    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_GPIO_TX, (gpio_num_t)CAN_GPIO_RX, modo);
    g.rx_queue_len = TWAI_FILA_RX_QUADROS;

    /* Os macros de timing são inicializadores, não expressões: por isso o if */
    twai_timing_config_t t;
    if (kbps == CAN_KBPS_BANCADA) {
        twai_timing_config_t t250 = TWAI_TIMING_CONFIG_250KBITS();
        t = t250;
    } else {
        twai_timing_config_t t500 = TWAI_TIMING_CONFIG_500KBITS();
        t = t500;
    }

    twai_filter_config_t f;
    if (filtrar_resposta_ecm) {
        /* Filtro único SJA1000, quadro padrão de 11 bits: o ID ocupa os bits
         * 31..21 do registrador de aceitação. Máscara com 1 = "não importa". */
        f.acceptance_code = OBD2_ID_RESPOSTA << 21;
        f.acceptance_mask = ~(0x7FFu << 21);
        f.single_filter = true;
    } else {
        f = (twai_filter_config_t)TWAI_FILTER_CONFIG_ACCEPT_ALL();
    }

    esp_err_t r = twai_driver_install(&g, &t, &f);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "twai_driver_install falhou: %s", esp_err_to_name(r));
        return r;
    }
    r = twai_start();
    if (r == ESP_OK) {
        s_instalado = true;
        s_kbps = kbps;
    }
    return r;
}

esp_err_t can_obd2_iniciar_escuta(uint16_t kbps)
{
    ESP_LOGI(TAG, "TWAI somente-escuta a %u kbit/s (invisivel para o barramento)", kbps);
    return instalar_driver(TWAI_MODE_LISTEN_ONLY, kbps, false);
}

can_obd2_escuta_t can_obd2_escutar(uint32_t janela_ms)
{
    can_obd2_escuta_t res = { 0 };
    twai_status_info_t st_ini = { 0 };
    twai_get_status_info(&st_ini);

    int64_t limite = agora_ms() + janela_ms;
    twai_message_t msg;
    while (agora_ms() < limite) {
        int64_t restante = limite - agora_ms();
        if (twai_receive(&msg, pdMS_TO_TICKS((uint32_t)restante)) == ESP_OK) {
            res.quadros_validos++;
            s_ultima_atividade_ms = agora_ms();
        }
    }

    /* O driver contabiliza cada interrupção de erro de barramento; a
     * diferença na janela é o que vimos de atividade "suja". */
    twai_status_info_t st_fim = { 0 };
    twai_get_status_info(&st_fim);
    res.erros_barramento = st_fim.bus_error_count - st_ini.bus_error_count;

    ESP_LOGI(TAG, "escuta %lu ms a %u kbit/s: %lu quadro(s) valido(s), %lu erro(s)",
             (unsigned long)janela_ms, s_kbps,
             (unsigned long)res.quadros_validos, (unsigned long)res.erros_barramento);
    return res;
}

esp_err_t can_obd2_modo_normal(void)
{
    ESP_LOGI(TAG, "TWAI em modo normal a %u kbit/s, filtro de HW para 0x%03X",
             s_kbps, OBD2_ID_RESPOSTA);
    esp_err_t r = instalar_driver(TWAI_MODE_NORMAL, s_kbps, true);
    if (r == ESP_OK) {
        /* Recomeça a contagem de silêncio a partir de agora */
        s_ultima_atividade_ms = agora_ms();
    }
    return r;
}

esp_err_t can_obd2_requisitar_pid(uint8_t pid, uint8_t *resposta,
                                  size_t *tamanho, uint32_t timeout_ms)
{
    twai_message_t msg;

    /* Drena respostas velhas que tenham ficado na fila (ex.: resposta que
     * chegou depois do timeout da requisição anterior) para não casar a
     * resposta errada com esta requisição. */
    while (twai_receive(&msg, 0) == ESP_OK) {
        s_ultima_atividade_ms = agora_ms();
    }

    /* ISO-TP single frame: PCI 0x02 (2 bytes de payload), modo, PID */
    twai_message_t req = {
        .identifier = OBD2_ID_REQUISICAO,
        .data_length_code = 8,
        .data = { 0x02, OBD2_MODO_DADOS, pid,
                  OBD2_PADDING, OBD2_PADDING, OBD2_PADDING, OBD2_PADDING, OBD2_PADDING },
    };
    esp_err_t r = twai_transmit(&req, pdMS_TO_TICKS(OBD2_TIMEOUT_TX_MS));
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "falha ao transmitir PID 0x%02X: %s", pid, esp_err_to_name(r));
        return r;
    }

    int64_t limite = agora_ms() + timeout_ms;
    while (agora_ms() < limite) {
        int64_t restante = limite - agora_ms();
        if (twai_receive(&msg, pdMS_TO_TICKS((uint32_t)restante)) != ESP_OK) {
            break; /* fila vazia até o limite — timeout */
        }
        s_ultima_atividade_ms = agora_ms();

        if (msg.identifier != OBD2_ID_RESPOSTA || msg.rtr) {
            continue;
        }
        /* Nibble alto do byte 0 = tipo de quadro ISO-TP. 0x0 = single frame.
         * First/consecutive frame (multi-frame) só aparece no Modo 03 — fase 2. */
        if ((msg.data[0] & 0xF0u) != 0x00u) {
            continue;
        }
        uint8_t payload = msg.data[0] & 0x0Fu;
        /* Resposta positiva do Modo 01: [len, 0x41, PID, A, B, ...] */
        if (payload < 2 || msg.data[1] != (0x40u | OBD2_MODO_DADOS) || msg.data[2] != pid) {
            continue; /* resposta de outro PID ou NRC (0x7F) — segue esperando */
        }
        size_t n = payload - 2;
        if (n > 5) {
            n = 5; /* single frame carrega no máximo 5 bytes de dados após 0x41+PID */
        }
        memcpy(resposta, &msg.data[3], n);
        *tamanho = n;
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

int64_t can_obd2_ultima_atividade_ms(void)
{
    return s_ultima_atividade_ms;
}

esp_err_t can_obd2_ler_dtcs(uint16_t *codigos, size_t maximo, size_t *quantidade)
{
    /* FASE 2 — STUB.
     * O Modo 03 responde com quantidade variável de DTCs: acima de 2 códigos
     * a resposta não cabe em single frame e vira multi-frame ISO-TP
     * (first frame 0x1X + flow control 0x30 enviado por nós + consecutive
     * frames 0x2X). Exige implementar o lado de flow control do ISO 15765-2,
     * que este esqueleto ainda não tem. A tela de DTC do Módulo B já existe
     * e mostra lista vazia até isto ser implementado. */
    (void)codigos;
    (void)maximo;
    *quantidade = 0;
    return ESP_ERR_NOT_SUPPORTED;
}
