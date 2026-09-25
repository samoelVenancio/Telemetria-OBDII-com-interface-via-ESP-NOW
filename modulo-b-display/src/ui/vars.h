/*
 * PLACEHOLDER — este arquivo será SOBRESCRITO pelo Build do EEZ Studio.
 *
 * Ele existe para o firmware compilar antes de o projeto EEZ ser criado, e
 * declara exatamente as "Native global variables" que o projeto EEZ precisa
 * ter (mesmos nomes e tipos). A implementação delas mora em ../ui_ponte.c.
 * Lista completa e passo a passo: ../../eez/LEIA-ME.md
 */
#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_NONE
};

// Native global variables

extern const char *get_var_rpm_txt();
extern void set_var_rpm_txt(const char *value);
extern int32_t get_var_rpm();
extern void set_var_rpm(int32_t value);
extern const char *get_var_vel_txt();
extern void set_var_vel_txt(const char *value);
extern const char *get_var_consumo_txt();
extern void set_var_consumo_txt(const char *value);
extern const char *get_var_consumo_unid_txt();
extern void set_var_consumo_unid_txt(const char *value);
extern const char *get_var_etanol_txt();
extern void set_var_etanol_txt(const char *value);
extern const char *get_var_temp_txt();
extern void set_var_temp_txt(const char *value);
extern const char *get_var_litros_txt();
extern void set_var_litros_txt(const char *value);
extern const char *get_var_tensao_txt();
extern void set_var_tensao_txt(const char *value);
extern bool get_var_alarme_oculto();
extern void set_var_alarme_oculto(bool value);
extern bool get_var_aviso_enlace_oculto();
extern void set_var_aviso_enlace_oculto(bool value);
extern const char *get_var_limiar_txt();
extern void set_var_limiar_txt(const char *value);
extern const char *get_var_histerese_txt();
extern void set_var_histerese_txt(const char *value);
extern const char *get_var_janela_txt();
extern void set_var_janela_txt(const char *value);
extern const char *get_var_media_txt();
extern void set_var_media_txt(const char *value);
extern const char *get_var_estado_alarme_txt();
extern void set_var_estado_alarme_txt(const char *value);
extern bool get_var_salvo_oculto();
extern void set_var_salvo_oculto(bool value);
extern const char *get_var_estado_enlace_txt();
extern void set_var_estado_enlace_txt(const char *value);
extern const char *get_var_rssi_txt();
extern void set_var_rssi_txt(const char *value);
extern const char *get_var_recebidos_txt();
extern void set_var_recebidos_txt(const char *value);
extern const char *get_var_perdidos_txt();
extern void set_var_perdidos_txt(const char *value);
extern const char *get_var_entrega_txt();
extern void set_var_entrega_txt(const char *value);
extern const char *get_var_taxa_txt();
extern void set_var_taxa_txt(const char *value);
extern const char *get_var_can_taxa_txt();
extern void set_var_can_taxa_txt(const char *value);
extern const char *get_var_can_firmware_txt();
extern void set_var_can_firmware_txt(const char *value);
extern const char *get_var_can_pedido_txt();
extern void set_var_can_pedido_txt(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/
