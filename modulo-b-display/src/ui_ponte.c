/**
 * ui_ponte.c — Liga os dados do sistema às telas desenhadas no EEZ Studio.
 *
 * Telas do projeto EEZ (modulo-b-display/eez/):
 *   principal        — menu: Settings, Main panel, Layout
 *   Painel principal — 6 campos configuráveis (Value_1..6) + linha de status
 *   Painel layout    — 6 listas: qual grandeza aparece em cada campo
 *   Painel Config    — taxa do CAN do Módulo A, versão, status do ESP-NOW
 *   Painel Alarms    — superaquecimento (limiar), motor frio, ECO (RPM de troca)
 *
 * Por que uma ponte e não código dentro de src/ui/:
 *  - src/ui/ é SOBRESCRITO a cada Build do EEZ; nada escrito à mão pode
 *    morar lá.
 *  - O EEZ gera tick_screen_x() chamando get_var_<nome>() e só redesenha o
 *    label se o texto mudou (strcmp). A ponte só mantém strings atualizadas.
 *  - O que o EEZ não liga sozinho (navegação dos botões, opções e eventos
 *    das listas) é ligado aqui, em ui_ponte_iniciar(), por cima dos objetos
 *    que o EEZ criou.
 *
 * Concorrência (a regra que evita travar o firmware):
 *  - Tudo aqui roda DENTRO da task do LVGL (core 1): o lv_timer de 10 Hz e
 *    os callbacks de eventos são disparados pelo lv_timer_handler.
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
#include "ui/screens.h"
#include "ui/vars.h"
#include "ui_ponte.h"

static const char *TAG = "ui_ponte";

#define FIRMWARE_VERSAO_B         "0.4.0"
#define PONTE_PERIODO_MS          100    /* 10 Hz — mesma cadência do enlace */
#define PONTE_PERIODO_LOG_MS      5000   /* resumo do enlace no serial */
#define PONTE_TIMEOUT_PEDIDO_MS   15000  /* Módulo A reinicia + detecta o CAN; folga generosa */
#define PONTE_TXT_MAX             40

/* Faixas dos sliders da tela Alarms (sobrepõem o que vier do EEZ) */
#define SLIDER_TEMP_MIN_C         80
#define SLIDER_TEMP_MAX_C         120
#define SLIDER_ECO_MIN_RPM        NVM_ECO_RPM_MIN
#define SLIDER_ECO_MAX_RPM        NVM_ECO_RPM_MAX
#define SLIDER_ECO_PASSO_RPM      100    /* arredonda para centenas: 3000, 3100... */

/* Motor frio: abaixo desta temperatura, giro acima do limite gasta o motor
 * (óleo ainda grosso, folgas de dilatação ainda não assentadas) */
#define MOTOR_FRIO_TEMP_D         600    /* 60,0 °C */
#define MOTOR_FRIO_RPM_MAX        3000

/* ---------------------------------------------------------------------------
 * Grandezas que podem ocupar um campo do painel. A ORDEM é a das opções nas
 * listas da tela Painel layout e é o que fica gravado na NVS — acrescentar
 * no fim é seguro; reordenar embaralha o layout salvo de quem já usa.
 * ------------------------------------------------------------------------- */
typedef enum {
    G_RPM = 0,
    G_VELOCIDADE,
    G_CONSUMO,
    G_ETANOL,
    G_TEMP_MOTOR,
    G_TEMP_AR,
    G_MAP,
    G_TANQUE_L,
    G_TANQUE_PCT,
    G_BATERIA,
    G_TOTAL,
} grandeza_t;

/* Sem acentos: as fontes Montserrat embutidas no LVGL não têm os glifos */
static const char OPCOES_GRANDEZAS[] =
    "RPM\nVelocidade\nConsumo\nEtanol\nTemp. motor\nTemp. ar\nMAP\n"
    "Tanque (L)\nTanque (%)\nBateria";

static const char OPCOES_CAN[] = "250 kbit/s\n500 kbit/s";
#define OPCAO_CAN_250K  0
#define OPCAO_CAN_500K  1

/* ---------------------------------------------------------------------------
 * Variáveis nativas do EEZ. Geram o par get/set que o vars.h declara. São só
 * de SAÍDA (dado -> tela): set_var_* existe porque o EEZ o declara, mas é
 * ignorado de propósito.
 * ------------------------------------------------------------------------- */
#define VAR_TEXTO(nome) \
    static char s_##nome[PONTE_TXT_MAX] = "--"; \
    const char *get_var_##nome(void) { return s_##nome; } \
    void set_var_##nome(const char *valor) { (void)valor; }

VAR_TEXTO(value_1)
VAR_TEXTO(value_2)
VAR_TEXTO(value_3)
VAR_TEXTO(value_4)
VAR_TEXTO(value_5)
VAR_TEXTO(value_6)
VAR_TEXTO(msg_status)
VAR_TEXTO(status_espnow)
VAR_TEXTO(rev_sys)
VAR_TEXTO(temp_max_alarm)
VAR_TEXTO(eco_mode_rpm)

static char *const s_campos[NVM_LAYOUT_CAMPOS] = {
    s_value_1, s_value_2, s_value_3, s_value_4, s_value_5, s_value_6,
};

/* ---------------------------------------------------------------------------
 * Estado interno
 * ------------------------------------------------------------------------- */
static uint8_t s_layout[NVM_LAYOUT_CAMPOS];
static lv_obj_t *s_lista_can = NULL;   /* bps_can_config no EEZ */
static uint16_t s_kbps_pedida = 0;     /* 0 = nenhum pedido pendente */
static int64_t s_pedido_ms = 0;

static int64_t agora_ms(void)
{
    return esp_timer_get_time() / 1000;
}

/* Escreve "12.3<unidade>" a partir de um inteiro escalado (casas = 1 ou 2),
 * cuidando do sinal de valores entre -1 e 0 (ex.: -0.5). */
static void fmt_escalado(char *dest, const char *prefixo, int32_t valor, int casas,
                         const char *unidade)
{
    int32_t divisor = (casas == 2) ? 100 : 10;
    const char *sinal = (valor < 0) ? "-" : "";
    int32_t abs_v = labs(valor);
    snprintf(dest, PONTE_TXT_MAX, (casas == 2) ? "%s%s%ld.%02ld%s" : "%s%s%ld.%ld%s",
             prefixo, sinal, (long)(abs_v / divisor), (long)(abs_v % divisor), unidade);
}

static void formatar_grandeza(char *dest, grandeza_t g, const telem_pacote_t *p)
{
    switch (g) {
    case G_RPM:
        snprintf(dest, PONTE_TXT_MAX, "%u rpm", (unsigned)p->rpm);
        break;
    case G_VELOCIDADE:
        snprintf(dest, PONTE_TXT_MAX, "%u km/h", (unsigned)p->velocidade);
        break;
    case G_CONSUMO:
        /* Contrato: kml_c == 0 significa parado/indefinido -> mostrar L/h */
        if (p->consumo_kml_c > 0) {
            fmt_escalado(dest, "", p->consumo_kml_c / 10, 1, " km/L");
        } else {
            fmt_escalado(dest, "", p->consumo_lh_c / 10, 1, " L/h");
        }
        break;
    case G_ETANOL:
        snprintf(dest, PONTE_TXT_MAX, "E%u", (unsigned)p->etanol_pct);
        break;
    case G_TEMP_MOTOR:
        fmt_escalado(dest, "Motor ", p->temp_arref_d, 1, "°C");
        break;
    case G_TEMP_AR:
        fmt_escalado(dest, "Ar ", p->iat_d, 1, "°C");
        break;
    case G_MAP:
        snprintf(dest, PONTE_TXT_MAX, "%u kPa", (unsigned)p->map_kpa);
        break;
    case G_TANQUE_L:
        fmt_escalado(dest, "", p->litros_d, 1, " L");
        break;
    case G_TANQUE_PCT:
        snprintf(dest, PONTE_TXT_MAX, "Tanque %u%%", (unsigned)p->nivel_pct);
        break;
    case G_BATERIA:
        fmt_escalado(dest, "", p->tensao_c, 2, " V");
        break;
    default:
        strcpy(dest, "--");
        break;
    }
}

/* ---------------------------------------------------------------------------
 * Atualização periódica (lv_timer a 10 Hz, dentro da task do LVGL)
 * ------------------------------------------------------------------------- */

static void atualizar_painel(const telem_pacote_t *p, bool tem, bool ok,
                             alarme_estado_t alarme)
{
    /* Linha de status: o problema mais grave primeiro. Sem enlace ou sem
     * dados do CAN, nenhum alarme pode ser avaliado — por isso vêm antes. */
    const nvm_config_t *cfg = nvm_config();
    bool dados = ok && (p->flags & TELEM_FLAG_DADOS_VALIDOS);
    if (!ok) {
        strcpy(s_msg_status, tem ? "Sem sinal do modulo A" : "Aguardando modulo A");
    } else if (!dados) {
        strcpy(s_msg_status, "Aguardando dados do CAN");
    } else if (cfg->alarme_temp_on && alarme == ALARME_ATIVO) {
        fmt_escalado(s_msg_status, "ALARME: motor a ", alarme_media_d(), 1, "°C");
    } else if (cfg->alarme_frio_on && p->temp_arref_d < MOTOR_FRIO_TEMP_D &&
               p->rpm > MOTOR_FRIO_RPM_MAX) {
        strcpy(s_msg_status, "Motor frio: alivie o giro");
    } else if (cfg->eco_on && p->velocidade > 0 && p->rpm > cfg->eco_rpm) {
        strcpy(s_msg_status, "ECO: troque de marcha");
    } else {
        strcpy(s_msg_status, "OK");
    }

    if (!ok) {
        return; /* congela os últimos valores; a linha de status já avisa */
    }
    for (int i = 0; i < NVM_LAYOUT_CAMPOS; i++) {
        formatar_grandeza(s_campos[i], (grandeza_t)s_layout[i], p);
    }
}

static void atualizar_config(const telem_pacote_t *p, const estado_enlace_t *e, bool ok)
{
    uint16_t kbps_atual = (p->flags & TELEM_FLAG_CAN_250K) ? 250 : 500;

    /* Pedido de troca de taxa em andamento? */
    if (s_kbps_pedida != 0) {
        if (ok && kbps_atual == s_kbps_pedida) {
            ESP_LOGI(TAG, "Modulo A confirmou CAN a %u kbit/s", kbps_atual);
            s_kbps_pedida = 0;
        } else if (agora_ms() - s_pedido_ms > PONTE_TIMEOUT_PEDIDO_MS) {
            ESP_LOGW(TAG, "Modulo A nao confirmou %u kbit/s", s_kbps_pedida);
            s_kbps_pedida = 0;
        }
    }

    /* A lista mostra a taxa REAL do Módulo A. Só é sincronizada quando o
     * usuário não está mexendo nela e não há pedido pendente. */
    if (s_lista_can != NULL && ok && s_kbps_pedida == 0 && !lv_dropdown_is_open(s_lista_can)) {
        uint32_t opcao = (kbps_atual == 250) ? OPCAO_CAN_250K : OPCAO_CAN_500K;
        if (lv_dropdown_get_selected(s_lista_can) != opcao) {
            lv_dropdown_set_selected(s_lista_can, opcao);
        }
    }

    if (s_kbps_pedida != 0) {
        snprintf(s_status_espnow, PONTE_TXT_MAX, "trocando CAN p/ %uk...", s_kbps_pedida);
    } else if (ok) {
        uint32_t total = e->recebidos + e->perdidos;
        uint32_t entrega = total ? (uint32_t)(((uint64_t)e->recebidos * 100u) / total) : 100;
        snprintf(s_status_espnow, PONTE_TXT_MAX, "OK %lu%% %d dBm",
                 (unsigned long)entrega, (int)e->rssi_dbm);
    } else {
        strcpy(s_status_espnow, "sem sinal");
    }

    snprintf(s_rev_sys, PONTE_TXT_MAX, "B %s | A %s", FIRMWARE_VERSAO_B,
             !ok ? "--" : (p->flags & TELEM_FLAG_BANCADA) ? "bancada" : "carro");
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

    atualizar_painel(&p, tem, ok, alarme);
    atualizar_config(&p, &enlace, ok);

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
 * Eventos ligados por cima dos objetos do EEZ
 * ------------------------------------------------------------------------- */

static void ao_navegar(lv_event_t *e)
{
    enum ScreensEnum destino = (enum ScreensEnum)(intptr_t)lv_event_get_user_data(e);
    loadScreen(destino);
}

static void ao_trocar_campo(lv_event_t *e)
{
    int campo = (int)(intptr_t)lv_event_get_user_data(e);
    uint32_t opcao = lv_dropdown_get_selected(lv_event_get_target_obj(e));
    if (opcao >= G_TOTAL) {
        return;
    }
    s_layout[campo] = (uint8_t)opcao;
    esp_err_t r = nvm_config_salvar_layout(s_layout);
    ESP_LOGI(TAG, "campo %d -> grandeza %lu (NVS: %s)", campo + 1,
             (unsigned long)opcao, esp_err_to_name(r));
}

static void pedir_taxa_can(uint16_t kbps)
{
    if (enlace_espnow_enviar_comando(TELEM_CMD_DEFINIR_CAN_KBPS, kbps) == ESP_OK) {
        s_kbps_pedida = kbps;
        s_pedido_ms = agora_ms();
    }
}

static void ao_trocar_can(lv_event_t *e)
{
    uint32_t opcao = lv_dropdown_get_selected(lv_event_get_target_obj(e));
    pedir_taxa_can(opcao == OPCAO_CAN_250K ? 250 : 500);
}

/* Retry: recomeça a medição de entrega e, se um pedido de taxa ficou sem
 * resposta, manda de novo. Não reinicia o rádio — com ESP-NOW em broadcast
 * não há conexão para refazer. */
static void ao_retry(lv_event_t *e)
{
    (void)e;
    estado_zerar_estatisticas();
    if (s_lista_can != NULL && s_kbps_pedida == 0) {
        telem_pacote_t p;
        if (estado_copiar(&p, NULL) && estado_enlace_ok()) {
            uint16_t na_lista = (lv_dropdown_get_selected(s_lista_can) == OPCAO_CAN_250K) ? 250 : 500;
            uint16_t no_a = (p.flags & TELEM_FLAG_CAN_250K) ? 250 : 500;
            if (na_lista != no_a) {
                pedir_taxa_can(na_lista);
            }
        }
    }
    ESP_LOGI(TAG, "Retry: estatisticas do enlace zeradas");
}

/* ---------- Tela Alarms ---------- */

static void atualizar_textos_alarmes(void)
{
    const nvm_config_t *cfg = nvm_config();
    snprintf(s_temp_max_alarm, PONTE_TXT_MAX, "%d °C", cfg->limiar_temp_d / 10);
    snprintf(s_eco_mode_rpm, PONTE_TXT_MAX, "%u rpm", (unsigned)cfg->eco_rpm);
}

static void salvar_config(const nvm_config_t *nova)
{
    esp_err_t r = nvm_config_salvar(nova);
    if (r == ESP_OK) {
        alarme_reconfigurar(); /* limiar novo vale já, com a média zerada */
    } else {
        ESP_LOGW(TAG, "config de alarmes rejeitada: %s", esp_err_to_name(r));
    }
    atualizar_textos_alarmes();
}

static uint16_t eco_do_slider(lv_obj_t *slider)
{
    int32_t v = lv_slider_get_value(slider);
    v = ((v + SLIDER_ECO_PASSO_RPM / 2) / SLIDER_ECO_PASSO_RPM) * SLIDER_ECO_PASSO_RPM;
    return (uint16_t)v;
}

/* Enquanto arrasta: só o texto acompanha (sem gravar na flash) */
static void ao_arrastar_temp(lv_event_t *e)
{
    snprintf(s_temp_max_alarm, PONTE_TXT_MAX, "%ld °C",
             (long)lv_slider_get_value(lv_event_get_target_obj(e)));
}

static void ao_arrastar_eco(lv_event_t *e)
{
    snprintf(s_eco_mode_rpm, PONTE_TXT_MAX, "%u rpm",
             (unsigned)eco_do_slider(lv_event_get_target_obj(e)));
}

/* Ao soltar: grava. Uma escrita por ajuste, não uma por pixel arrastado. */
static void ao_soltar_temp(lv_event_t *e)
{
    nvm_config_t nova = *nvm_config();
    nova.limiar_temp_d = (int16_t)(lv_slider_get_value(lv_event_get_target_obj(e)) * 10);
    salvar_config(&nova);
}

static void ao_soltar_eco(lv_event_t *e)
{
    nvm_config_t nova = *nvm_config();
    nova.eco_rpm = eco_do_slider(lv_event_get_target_obj(e));
    salvar_config(&nova);
}

/* Switches: user_data diz qual campo da config o switch controla */
enum { SW_TEMP = 0, SW_FRIO, SW_ECO };

static void ao_trocar_switch(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target_obj(e);
    uint8_t ligado = lv_obj_has_state(sw, LV_STATE_CHECKED) ? 1 : 0;
    nvm_config_t nova = *nvm_config();
    switch ((int)(intptr_t)lv_event_get_user_data(e)) {
    case SW_TEMP: nova.alarme_temp_on = ligado; break;
    case SW_FRIO: nova.alarme_frio_on = ligado; break;
    case SW_ECO:  nova.eco_on = ligado; break;
    default: return;
    }
    salvar_config(&nova);
}

static void preparar_switch(lv_obj_t *sw, bool ligado, int qual)
{
    if (sw == NULL) {
        return;
    }
    if (ligado) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(sw, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sw, ao_trocar_switch, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)qual);
}

static void preparar_slider(lv_obj_t *sl, int32_t min, int32_t max, int32_t valor,
                            lv_event_cb_t ao_arrastar, lv_event_cb_t ao_soltar)
{
    if (sl == NULL) {
        return;
    }
    lv_slider_set_range(sl, min, max);
    lv_slider_set_value(sl, valor, LV_ANIM_OFF);
    lv_obj_add_event_cb(sl, ao_arrastar, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sl, ao_soltar, LV_EVENT_RELEASED, NULL);
}

static void preparar_tela_alarmes(void)
{
    const nvm_config_t *cfg = nvm_config();
    preparar_switch(objects.max_temp_alarm_sw, cfg->alarme_temp_on, SW_TEMP);
    preparar_switch(objects.cold_engine_alarm_sw, cfg->alarme_frio_on, SW_FRIO);
    preparar_switch(objects.eco_mode_sw, cfg->eco_on, SW_ECO);
    preparar_slider(objects.slide_max_temp, SLIDER_TEMP_MIN_C, SLIDER_TEMP_MAX_C,
                    cfg->limiar_temp_d / 10, ao_arrastar_temp, ao_soltar_temp);
    preparar_slider(objects.slide_eco_mode_rpm, SLIDER_ECO_MIN_RPM, SLIDER_ECO_MAX_RPM,
                    cfg->eco_rpm, ao_arrastar_eco, ao_soltar_eco);
    atualizar_textos_alarmes();
}

/* Ícones do EEZ exportados em A8 (só alfa, sem cor): o LVGL 9 desenha A8 com
 * a cor de "image recolor" do estilo, que por padrão é PRETA — no fundo
 * preto das telas o ícone fica invisível, embora clicável. Aqui a cor é
 * definida explicitamente, com um cinza no toque como resposta visual.
 * A8 é o formato certo para ícones monocromáticos: 1 byte por pixel (4x
 * menos que ARGB8888) e cor escolhida em tempo de execução. */
#define COR_ICONE          lv_color_white()
#define COR_ICONE_TOCADO   lv_color_hex(0x8A96A3)

static void colorir_icone(lv_obj_t *obj)
{
    if (obj == NULL) {
        return;
    }
    lv_obj_set_style_image_recolor(obj, COR_ICONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor(obj, COR_ICONE_TOCADO, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_image_recolor_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
}

static void ligar_botao(lv_obj_t *botao, lv_event_cb_t cb, intptr_t dado)
{
    if (botao != NULL) {
        lv_obj_add_event_cb(botao, cb, LV_EVENT_CLICKED, (void *)dado);
    }
}

/* ------------------------------------------------------------------------- */

void ui_ponte_iniciar(void)
{
    /* Layout salvo, com sanidade: índice fora da tabela volta ao padrão RPM */
    memcpy(s_layout, nvm_config()->layout, sizeof(s_layout));
    for (int i = 0; i < NVM_LAYOUT_CAMPOS; i++) {
        if (s_layout[i] >= G_TOTAL) {
            s_layout[i] = G_RPM;
        }
    }

    /* Navegação: menu -> telas, Return -> menu */
    ligar_botao(objects.btn_settings, ao_navegar, SCREEN_ID_PAINEL_CONFIG);
    ligar_botao(objects.btn_panel, ao_navegar, SCREEN_ID_PAINEL_PRINCIPAL);
    ligar_botao(objects.btn_layout, ao_navegar, SCREEN_ID_PAINEL_LAYOUT);
    ligar_botao(objects.return_painel, ao_navegar, SCREEN_ID_PRINCIPAL);
    ligar_botao(objects.return_layout, ao_navegar, SCREEN_ID_PRINCIPAL);
    ligar_botao(objects.return_config, ao_navegar, SCREEN_ID_PRINCIPAL);
    ligar_botao(objects.btn_alarms, ao_navegar, SCREEN_ID_PAINEL_ALARMS);
    ligar_botao(objects.return_alarms, ao_navegar, SCREEN_ID_PRINCIPAL);

    /* Botões de voltar viraram ícones (imagebutton A8) no EEZ */
    colorir_icone(objects.return_painel);
    colorir_icone(objects.return_layout);
    colorir_icone(objects.return_config);
    colorir_icone(objects.return_alarms);

    preparar_tela_alarmes();

    /* Listas do Painel layout: opções reais + seleção salva na NVS */
    lv_obj_t *listas[NVM_LAYOUT_CAMPOS] = {
        objects.dropdown_v1, objects.dropdown_v2, objects.dropdown_v3,
        objects.dropdown_v4, objects.dropdown_v5, objects.dropdown_v6,
    };
    for (int i = 0; i < NVM_LAYOUT_CAMPOS; i++) {
        if (listas[i] == NULL) {
            continue;
        }
        lv_dropdown_set_options_static(listas[i], OPCOES_GRANDEZAS);
        lv_dropdown_set_selected(listas[i], s_layout[i]);
        lv_obj_add_event_cb(listas[i], ao_trocar_campo, LV_EVENT_VALUE_CHANGED,
                            (void *)(intptr_t)i);
    }

    /* Tela Config: lista da taxa do CAN e botão Retry */
    s_lista_can = objects.bps_can_config;
    if (s_lista_can != NULL) {
        lv_dropdown_set_options_static(s_lista_can, OPCOES_CAN);
        lv_dropdown_set_selected(s_lista_can, OPCAO_CAN_500K); /* padrão de fábrica do A */
        lv_obj_add_event_cb(s_lista_can, ao_trocar_can, LV_EVENT_VALUE_CHANGED, NULL);
    }
    ligar_botao(objects.retry_config, ao_retry, 0);

    snprintf(s_rev_sys, PONTE_TXT_MAX, "B %s | A --", FIRMWARE_VERSAO_B);

    lv_timer_create(ao_disparar_timer, PONTE_PERIODO_MS, NULL);
    ESP_LOGI(TAG, "ponte EEZ ativa, atualizacao a %d ms", PONTE_PERIODO_MS);
}
