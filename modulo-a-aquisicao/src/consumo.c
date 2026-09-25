/**
 * consumo.c — Consumo instantâneo calculado por speed-density.
 *
 * Este veículo NÃO expõe MAF (0x10) nem vazão de combustível (0x5E) — foi
 * confirmado por varredura real. O motor Sigma 1.6 flex trabalha com
 * estratégia speed-density: a massa de ar é estimada pela equação dos gases
 * ideais a partir de RPM, MAP e IAT. Reproduzir essa estimativa fora da ECU
 * e derivar o consumo é a contribuição central deste projeto.
 *
 * Fórmulas (não alterar sem refazer a validação):
 *   IAT_K   = iat_c + 273.15
 *   AFR     = 14.7 − (14.7 − 9.0) · (etanol_pct / 100)      [interp. linear]
 *   rho     = 0.745 + (0.809 − 0.745) · (etanol_pct / 100)  [kg/L]
 *   MAF_gs  = (rpm · map_kpa · VE · Vd · 28.97) / (120 · 8.314 · IAT_K)
 *   comb_gs = MAF_gs / AFR
 *   comb_lh = (comb_gs · 3600) / (rho · 1000)
 *   km_por_L = velocidade / comb_lh                          [se comb_lh > 0]
 *
 * O fator 120 embute: motor 4 tempos (1 admissão a cada 2 voltas) e a
 * conversão rpm -> rev/s (60), com kPa e litros se resolvendo nas demais
 * constantes (28.97 g/mol do ar, R = 8.314 J/(mol·K)).
 *
 * Floats são usados livremente AQUI DENTRO: a proibição de float vale para o
 * pacote no ar (contrato), não para o cálculo local.
 */
#include <math.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "consumo.h"

static const char *TAG = "consumo";

/* Constantes físicas e do veículo — unidade no nome */
#define CILINDRADA_L          1.596f   /* Vd: Sigma 1.6 do New Fiesta (1596 cm³) */
#define AFR_GASOLINA          14.7f    /* estequiométrico E0 (a gasolina BR é E27, coberto pela interp.) */
#define AFR_ETANOL            9.0f     /* estequiométrico E100 */
#define RHO_GASOLINA_KG_L     0.745f
#define RHO_ETANOL_KG_L       0.809f
#define MASSA_MOLAR_AR_G_MOL  28.97f
#define R_UNIVERSAL_J_MOL_K   8.314f
#define FATOR_4T_RPM_S        120.0f   /* 4 tempos: 2 voltas/admissão × 60 s/min */

#define KML_MAXIMO_EXIBIVEL   99.99f   /* saturação p/ caber em uint16 ×100 e fazer sentido no display */
#define IAT_K_MINIMO_SANIDADE 200.0f   /* -73 °C: abaixo disso o dado é lixo, não clima */

/* VE (eficiência volumétrica) — NÃO é constante de código.
 *
 * VE varia com carga, rotação e estado do motor; usar um único escalar é uma
 * aproximação de ordem zero que PRECISA de calibração empírica: comparar o
 * consumo médio integrado pelo firmware contra abastecimentos reais
 * (tanque cheio a tanque cheio) e ajustar o VE até as médias baterem.
 * Por isso ele mora na NVS (ajustável em campo, sobrevive a reboot) e não
 * num #define. Valor inicial 0.85 é típico de motor aspirado moderno.
 * Evolução natural (fase 2): tabela VE(rpm, MAP) em vez de escalar. */
#define VE_PADRAO             0.85f
#define VE_MINIMO             0.30f
#define VE_MAXIMO             1.10f

#define NVS_NAMESPACE         "telemetria"
#define NVS_CHAVE_VE          "ve_milesimos"  /* uint16, VE × 1000 */

static float s_ve = VE_PADRAO;

void consumo_iniciar(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK) {
        uint16_t milesimos = 0;
        if (nvs_get_u16(h, NVS_CHAVE_VE, &milesimos) == ESP_OK && milesimos > 0) {
            s_ve = milesimos / 1000.0f;
        }
        nvs_close(h);
    }
    ESP_LOGI(TAG, "VE em uso: %.3f", (double)s_ve);
}

float consumo_ve(void)
{
    return s_ve;
}

esp_err_t consumo_definir_ve(float ve)
{
    if (ve < VE_MINIMO || ve > VE_MAXIMO) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (r != ESP_OK) {
        return r;
    }
    r = nvs_set_u16(h, NVS_CHAVE_VE, (uint16_t)(ve * 1000.0f + 0.5f));
    if (r == ESP_OK) {
        r = nvs_commit(h);
    }
    nvs_close(h);
    if (r == ESP_OK) {
        s_ve = ve;
        ESP_LOGI(TAG, "novo VE gravado: %.3f", (double)ve);
    }
    return r;
}

consumo_saida_t consumo_calcular(const dados_veiculo_t *d)
{
    consumo_saida_t s = { .kml_c = 0, .lh_c = 0, .valido = false };

    /* Insumos obrigatórios do speed-density. Sem qualquer um deles o
     * resultado seria chute, então o cálculo é marcado inválido inteiro. */
    if (!d->rpm_valido || !d->map_valido || !d->iat_valida || !d->etanol_valido) {
        return s;
    }
    /* Motor parado: não há admissão, consumo é zero por definição —
     * e RPM no denominador implícito não faz sentido. */
    if (d->rpm == 0) {
        return s;
    }

    float iat_k = (d->iat_d / 10.0f) + 273.15f;
    if (iat_k < IAT_K_MINIMO_SANIDADE) {
        return s; /* proteção contra divisão por valor absurdo/negativo */
    }

    float fracao_etanol = d->etanol_pct / 100.0f;
    float afr = AFR_GASOLINA - (AFR_GASOLINA - AFR_ETANOL) * fracao_etanol;
    float rho = RHO_GASOLINA_KG_L + (RHO_ETANOL_KG_L - RHO_GASOLINA_KG_L) * fracao_etanol;

    float maf_gs = ((float)d->rpm * (float)d->map_kpa * s_ve * CILINDRADA_L
                    * MASSA_MOLAR_AR_G_MOL)
                   / (FATOR_4T_RPM_S * R_UNIVERSAL_J_MOL_K * iat_k);

    float comb_gs = maf_gs / afr;
    float comb_lh = (comb_gs * 3600.0f) / (rho * 1000.0f);

    /* Corte de injeção (desaceleração engatada): a ECU zera o combustível mas
     * o ar continua entrando, então este modelo superestima o consumo nessa
     * condição. TODO fase 2: detectar corte (MAP baixo + RPM alto + pedal
     * solto) e forçar comb_lh = 0 — hoje o valor mostrado em freio-motor é
     * o limite superior, não o real. */

    if (comb_lh < 0.0f) {
        comb_lh = 0.0f;
    }
    s.lh_c = (uint16_t)(comb_lh * 100.0f + 0.5f);

    /* Parado (ou velocidade inválida): km/L não é definido — o display mostra
     * L/h. O sinal disso no contrato é kml_c == 0. Também protege a divisão. */
    if (d->velocidade_valida && d->velocidade_kmh > 0 && comb_lh > 0.01f) {
        float kml = (float)d->velocidade_kmh / comb_lh;
        if (kml > KML_MAXIMO_EXIBIVEL) {
            kml = KML_MAXIMO_EXIBIVEL;
        }
        s.kml_c = (uint16_t)(kml * 100.0f + 0.5f);
    }

    s.valido = true;
    return s;
}
