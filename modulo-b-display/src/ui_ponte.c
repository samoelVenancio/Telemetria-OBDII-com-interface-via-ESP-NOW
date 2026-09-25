/**
 * ui_ponte.c — Implementa as variáveis nativas e as actions da UI do EEZ.
 *
 * Por que uma ponte e não código dentro de src/ui/:
 *  - src/ui/ é SOBRESCRITO a cada Build do EEZ Studio; nada escrito à mão
 *    pode morar lá.
 *  - O EEZ, em projeto LVGL sem Flow, gera tick_screen_x() que chama
 *    get_var_<nome>() e só invalida o widget se o texto mudou (strcmp). Então
 *    a ponte só precisa manter strings formatadas atualizadas; a guarda
 *    "mudou?" já vem de graça do código gerado.
 *
 * Concorrência (a regra que evita travar o firmware):
 *  - Tudo aqui roda DENTRO da task do LVGL (core 1): o lv_timer de 10 Hz e
 *    os callbacks das actions são disparados pelo lv_timer_handler.
 *  - O dado de rede chega por estado_copiar(), que copia sob spinlock o que
 *    o callback do ESP-NOW (core 0) deixou lá. O callback nunca toca em LVGL.
 *
 * Toda formatação é em inteiro (o contrato não tem float e o LVGL está com
 * LV_SPRINTF_USE_FLOAT desligado).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

#include "telemetria_protocolo.h"
#include "estado.h"
#include "alarme.h"
#include "nvm_config.h"
#include "enlace_espnow.h"
#include "ui/ui.h"
#include "ui/vars.h"
#include "ui/actions.h"
#include "ui_ponte.h"

static const char *TAG = "ui_ponte";

#define PONTE_PERIODO_MS          100    /* 10 Hz — mesma cadência do enlace */
#define PONTE_JANELA_TAXA_MS      1000   /* taxa efetiva medida a cada 1 s */
#define PONTE_TIMEOUT_PEDIDO_MS   15000  /* Módulo A reinicia + escuta 3 s; folga generosa */
#define PONTE_TXT_MAX             40
#define PONTE_PERIODO_LOG_MS      5000   /* resumo do enlace no serial */
#define PONTE_RPM_ESCALA_MAXIMA   7000

/* Passos dos ajustes do alarme por toque */
#define PASSO_LIMIAR_D     5   /* 0,5 °C */
#define PASSO_HISTERESE_D  5   /* 0,5 °C */
#define PASSO_JANELA       1   /* 1 amostra */

/* ---------------------------------------------------------------------------
 * Variáveis nativas. Os macros geram o par get/set que o vars.h do EEZ
 * declara. As variáveis são só de SAÍDA (dado -> tela): set_var_* existe
 * porque o EEZ o declara, mas é ignorado de propósito.
 * ------------------------------------------------------------------------- */
#define VAR_TEXTO(nome) \
    static char s_##nome[PONTE_TXT_MAX] = "--"; \
    const char *get_var_##nome(void) { return s_##nome; } \
    void set_var_##nome(const char *valor) { (void)valor; }

#define VAR_BOOL(nome, inicial) \
    static bool s_##nome = (inicial); \
    bool get_var_##nome(void) { return s_##nome; } \
    void set_var_##nome(bool valor) { (void)valor; }

#define VAR_INT(nome) \
    static int32_t s_##nome = 0; \
    int32_t get_var_##nome(void) { return s_##nome; } \
    void set_var_##nome(int32_t valor) { (void)valor; }

/* Principal */
VAR_TEXTO(rpm_txt)
VAR_INT(rpm)
VAR_TEXTO(vel_txt)
VAR_TEXTO(consumo_txt)
VAR_TEXTO(consumo_unid_txt)
VAR_TEXTO(etanol_txt)
VAR_TEXTO(temp_txt)
VAR_TEXTO(litros_txt)
VAR_TEXTO(tensao_txt)
VAR_BOOL(alarme_oculto, true)
VAR_BOOL(aviso_enlace_oculto, false)
/* Alarme */
VAR_TEXTO(limiar_txt)
VAR_TEXTO(histerese_txt)
VAR_TEXTO(janela_txt)
VAR_TEXTO(media_txt)
VAR_TEXTO(estado_alarme_txt)
VAR_BOOL(salvo_oculto, true)
/* Enlace */
VAR_TEXTO(estado_enlace_txt)
VAR_TEXTO(rssi_txt)
VAR_TEXTO(recebidos_txt)
VAR_TEXTO(perdidos_txt)
VAR_TEXTO(entrega_txt)
VAR_TEXTO(taxa_txt)
/* Configuração */
VAR_TEXTO(can_taxa_txt)
VAR_TEXTO(can_firmware_txt)
VAR_TEXTO(can_pedido_txt)

/* ---------------------------------------------------------------------------
 * Estado interno da ponte
 * ------------------------------------------------------------------------- */
static nvm_config_t s_edicao;          /* cópia em edição na tela de alarme */
static uint16_t s_kbps_pedida = 0;     /* 0 = nenhum pedido pendente */
static int64_t s_pedido_ms = 0;
static int64_t s_taxa_inicio_ms = 0;
static uint32_t s_taxa_base = 0;

static int64_t agora_ms(void)
{
    return esp_timer_get_time() / 1000;
}

/* Escreve "12.3" a partir de um inteiro escalado (casas = 1 ou 2),
 * cuidando do sinal de valores negativos entre -1 e 0 (ex.: -0.5). */
static void fmt_escalado(char *dest, int32_t valor, int casas, const char *unidade)
{
    int32_t divisor = (casas == 2) ? 100 : 10;
    const char *sinal = (valor < 0) ? "-" : "";
    int32_t abs_v = labs(valor);
    snprintf(dest, PONTE_TXT_MAX, (casas == 2) ? "%s%ld.%02ld%s" : "%s%ld.%ld%s",
             sinal, (long)(abs_v / divisor), (long)(abs_v % divisor), unidade);
}

static void atualizar_textos_edicao(void)
{
    fmt_escalado(s_limiar_txt, s_edicao.limiar_temp_d, 1, " °C");
    fmt_escalado(s_histerese_txt, s_edicao.histerese_d, 1, " °C");
    snprintf(s_janela_txt, PONTE_TXT_MAX, "%u amostras", (unsigned)s_edicao.janela_media);
}

/* ---------------------------------------------------------------------------
 * Atualização periódica (lv_timer a 10 Hz, dentro da task do LVGL)
 * ------------------------------------------------------------------------- */

static void atualizar_principal(const telem_pacote_t *p, bool tem, bool ok,
                                alarme_estado_t alarme)
{
    s_aviso_enlace_oculto = ok;
    s_alarme_oculto = (alarme != ALARME_ATIVO);
    if (!tem || !ok) {
        return; /* congela os últimos valores; o aviso de enlace já diz que são velhos */
    }

    snprintf(s_rpm_txt, PONTE_TXT_MAX, "%u", (unsigned)p->rpm);
    s_rpm = (p->rpm > PONTE_RPM_ESCALA_MAXIMA) ? PONTE_RPM_ESCALA_MAXIMA : p->rpm;
    snprintf(s_vel_txt, PONTE_TXT_MAX, "%u", (unsigned)p->velocidade);

    /* Contrato: kml_c == 0 significa parado/indefinido -> mostrar L/h */
    if (p->consumo_kml_c > 0) {
        fmt_escalado(s_consumo_txt, p->consumo_kml_c / 10, 1, "");
        strcpy(s_consumo_unid_txt, "km/L");
    } else {
        fmt_escalado(s_consumo_txt, p->consumo_lh_c / 10, 1, "");
        strcpy(s_consumo_unid_txt, "L/h");
    }

    snprintf(s_etanol_txt, PONTE_TXT_MAX, "%u %%", (unsigned)p->etanol_pct);
    fmt_escalado(s_temp_txt, p->temp_arref_d, 1, " °C");
    fmt_escalado(s_litros_txt, p->litros_d, 1, " L");
    fmt_escalado(s_tensao_txt, p->tensao_c, 2, " V");
}

static void atualizar_alarme(alarme_estado_t alarme)
{
    if (alarme == ALARME_SEM_DADOS) {
        strcpy(s_media_txt, "--");
        strcpy(s_estado_alarme_txt, "SEM DADOS");
    } else {
        fmt_escalado(s_media_txt, alarme_media_d(), 1, " °C");
        strcpy(s_estado_alarme_txt, (alarme == ALARME_ATIVO) ? "ALARME" : "NORMAL");
    }
}

static void atualizar_enlace(const estado_enlace_t *e, bool tem, bool ok)
{
    strcpy(s_estado_enlace_txt, ok ? "CONECTADO" : "SEM SINAL");
    if (!tem) {
        return;
    }
    snprintf(s_rssi_txt, PONTE_TXT_MAX, "%d dBm", (int)e->rssi_dbm);
    snprintf(s_recebidos_txt, PONTE_TXT_MAX, "%lu", (unsigned long)e->recebidos);
    snprintf(s_perdidos_txt, PONTE_TXT_MAX, "%lu", (unsigned long)e->perdidos);

    uint32_t total = e->recebidos + e->perdidos;
    if (total > 0) {
        snprintf(s_entrega_txt, PONTE_TXT_MAX, "%lu %%",
                 (unsigned long)(((uint64_t)e->recebidos * 100u) / total));
    }

    /* Taxa efetiva: pacotes contados numa janela de 1 s (nominal: 10/s) */
    int64_t agora = agora_ms();
    if (s_taxa_inicio_ms == 0) {
        s_taxa_inicio_ms = agora;
        s_taxa_base = e->recebidos;
    } else if (agora - s_taxa_inicio_ms >= PONTE_JANELA_TAXA_MS) {
        uint32_t decimos_hz = (uint32_t)(((uint64_t)(e->recebidos - s_taxa_base) * 10000u)
                                         / (uint64_t)(agora - s_taxa_inicio_ms));
        snprintf(s_taxa_txt, PONTE_TXT_MAX, "%lu.%lu pct/s",
                 (unsigned long)(decimos_hz / 10), (unsigned long)(decimos_hz % 10));
        s_taxa_inicio_ms = agora;
        s_taxa_base = e->recebidos;
    }
}

static void atualizar_config(const telem_pacote_t *p, bool ok)
{
    uint16_t kbps_atual = (p->flags & TELEM_FLAG_CAN_250K) ? 250 : 500;

    if (ok) {
        snprintf(s_can_taxa_txt, PONTE_TXT_MAX, "%u kbit/s", kbps_atual);
        strcpy(s_can_firmware_txt, (p->flags & TELEM_FLAG_BANCADA) ? "bancada" : "carro");
    } else {
        strcpy(s_can_taxa_txt, "sem enlace");
        strcpy(s_can_firmware_txt, "--");
    }

    if (s_kbps_pedida == 0) {
        return;
    }
    if (ok && kbps_atual == s_kbps_pedida) {
        snprintf(s_can_pedido_txt, PONTE_TXT_MAX, "%u kbit/s aplicado", s_kbps_pedida);
        s_kbps_pedida = 0;
    } else if (agora_ms() - s_pedido_ms > PONTE_TIMEOUT_PEDIDO_MS) {
        /* Módulo A não voltou na taxa nova: dormindo (sem tráfego) ou fora de alcance */
        snprintf(s_can_pedido_txt, PONTE_TXT_MAX, "%u kbit/s sem confirmacao", s_kbps_pedida);
        s_kbps_pedida = 0;
    }
}

static void ao_disparar_timer(lv_timer_t *timer)
{
    (void)timer;

    telem_pacote_t p;
    estado_enlace_t enlace;
    bool tem = estado_copiar(&p, &enlace);
    bool ok = tem && estado_enlace_ok();

    /* O alarme é alimentado aqui (cadência fixa de 10 Hz) e não na recepção:
     * a média móvel precisa de base de tempo estável. */
    bool amostra_valida = ok && (p.flags & TELEM_FLAG_DADOS_VALIDOS);
    alarme_estado_t alarme = alarme_processar(p.temp_arref_d, amostra_valida);

    atualizar_principal(&p, tem, ok, alarme);
    atualizar_alarme(alarme);
    atualizar_enlace(&enlace, tem, ok);
    atualizar_config(&p, ok);

    /* Resumo no serial a cada 5 s: mostra se o enlace está vivo sem precisar
     * olhar a tela. Log a 0,2 Hz na task do LVGL não atrapalha o desenho. */
    static int64_t ultimo_log_ms = 0;
    int64_t agora = agora_ms();
    if (agora - ultimo_log_ms >= PONTE_PERIODO_LOG_MS) {
        ultimo_log_ms = agora;
        ESP_LOGI(TAG, "enlace %s | rx %lu perdidos %lu invalidos %lu rssi %d | rpm %u dados %s",
                 ok ? "OK" : "SEM SINAL",
                 (unsigned long)enlace.recebidos, (unsigned long)enlace.perdidos,
                 (unsigned long)enlace.invalidos, (int)enlace.rssi_dbm, p.rpm,
                 (p.flags & TELEM_FLAG_DADOS_VALIDOS) ? "validos" : "INVALIDOS");
    }

    /* Código gerado pelo EEZ: lê os get_var_*() da tela atual e só redesenha
     * o que mudou */
    ui_tick();
}

/* ---------------------------------------------------------------------------
 * Actions (ligadas no EEZ ao evento CLICKED dos botões)
 * ------------------------------------------------------------------------- */

/* Navegação por POSIÇÃO da tela no projeto EEZ (1 = primeira da lista), e
 * não por SCREEN_ID_<NOME>: assim o firmware compila com qualquer número de
 * telas no EEZ. O EEZ sempre gera _SCREEN_ID_LAST; ir para uma tela que
 * ainda não existe vira no-op (sem isso, loadScreen indexaria um widget
 * qualquer da struct objects como se fosse tela e o firmware travaria). */
enum {
    POS_TELA_PRINCIPAL = 1,
    POS_TELA_ALARME    = 2,
    POS_TELA_DTC       = 3,
    POS_TELA_ENLACE    = 4,
    POS_TELA_CONFIG    = 5,
};

static void ir_para(int posicao)
{
    if (posicao >= 1 && posicao <= _SCREEN_ID_LAST) {
        loadScreen((enum ScreensEnum)posicao);
    } else {
        ESP_LOGW(TAG, "tela %d ainda nao existe no projeto EEZ", posicao);
    }
}

void action_ir_principal(lv_event_t *e) { (void)e; ir_para(POS_TELA_PRINCIPAL); }
void action_ir_alarme(lv_event_t *e)    { (void)e; ir_para(POS_TELA_ALARME); }
void action_ir_dtc(lv_event_t *e)       { (void)e; ir_para(POS_TELA_DTC); }
void action_ir_enlace(lv_event_t *e)    { (void)e; ir_para(POS_TELA_ENLACE); }
void action_ir_config(lv_event_t *e)    { (void)e; ir_para(POS_TELA_CONFIG); }

static int16_t limitar_i16(int32_t v, int16_t minimo, int16_t maximo)
{
    if (v < minimo) return minimo;
    if (v > maximo) return maximo;
    return (int16_t)v;
}

/* Os ajustes editam uma CÓPIA; só "Salvar" grava e reconfigura o alarme.
 * Editar a configuração viva faria o alarme mudar no meio do ajuste. */
static void apos_ajuste(void)
{
    atualizar_textos_edicao();
    s_salvo_oculto = true;
}

void action_limiar_menos(lv_event_t *e)
{
    (void)e;
    s_edicao.limiar_temp_d = limitar_i16(s_edicao.limiar_temp_d - PASSO_LIMIAR_D,
                                         NVM_LIMIAR_TEMP_MIN_D, NVM_LIMIAR_TEMP_MAX_D);
    apos_ajuste();
}

void action_limiar_mais(lv_event_t *e)
{
    (void)e;
    s_edicao.limiar_temp_d = limitar_i16(s_edicao.limiar_temp_d + PASSO_LIMIAR_D,
                                         NVM_LIMIAR_TEMP_MIN_D, NVM_LIMIAR_TEMP_MAX_D);
    apos_ajuste();
}

void action_histerese_menos(lv_event_t *e)
{
    (void)e;
    s_edicao.histerese_d = limitar_i16(s_edicao.histerese_d - PASSO_HISTERESE_D,
                                       NVM_HISTERESE_MIN_D, NVM_HISTERESE_MAX_D);
    apos_ajuste();
}

void action_histerese_mais(lv_event_t *e)
{
    (void)e;
    s_edicao.histerese_d = limitar_i16(s_edicao.histerese_d + PASSO_HISTERESE_D,
                                       NVM_HISTERESE_MIN_D, NVM_HISTERESE_MAX_D);
    apos_ajuste();
}

void action_janela_menos(lv_event_t *e)
{
    (void)e;
    if (s_edicao.janela_media > NVM_JANELA_MIN) {
        s_edicao.janela_media -= PASSO_JANELA;
    }
    apos_ajuste();
}

void action_janela_mais(lv_event_t *e)
{
    (void)e;
    if (s_edicao.janela_media < NVM_JANELA_MAX) {
        s_edicao.janela_media += PASSO_JANELA;
    }
    apos_ajuste();
}

void action_salvar_alarme(lv_event_t *e)
{
    (void)e;
    if (nvm_config_salvar(&s_edicao) == ESP_OK) {
        alarme_reconfigurar();
        s_salvo_oculto = false;
    }
}

static void pedir_taxa_can(uint16_t kbps)
{
    if (enlace_espnow_enviar_comando(TELEM_CMD_DEFINIR_CAN_KBPS, kbps) == ESP_OK) {
        s_kbps_pedida = kbps;
        s_pedido_ms = agora_ms();
        snprintf(s_can_pedido_txt, PONTE_TXT_MAX, "enviado %u kbit/s, aguardando...", kbps);
    } else {
        strcpy(s_can_pedido_txt, "falha ao enviar");
    }
}

void action_can_250k(lv_event_t *e) { (void)e; pedir_taxa_can(250); }
void action_can_500k(lv_event_t *e) { (void)e; pedir_taxa_can(500); }

/* ------------------------------------------------------------------------- */

void ui_ponte_iniciar(void)
{
    s_edicao = *nvm_config();
    atualizar_textos_edicao();
    strcpy(s_can_pedido_txt, "--");

    lv_timer_create(ao_disparar_timer, PONTE_PERIODO_MS, NULL);
    ESP_LOGI(TAG, "ponte EEZ ativa, atualizacao a %d ms", PONTE_PERIODO_MS);
}
