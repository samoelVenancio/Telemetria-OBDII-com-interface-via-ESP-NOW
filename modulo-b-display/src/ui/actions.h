/*
 * PLACEHOLDER — este arquivo será SOBRESCRITO pelo Build do EEZ Studio.
 *
 * Declara as "Actions" que o projeto EEZ precisa ter (mesmos nomes). Cada
 * uma é ligada no EEZ ao evento CLICKED de um botão; a implementação mora
 * em ../ui_ponte.c. Lista completa: ../../eez/LEIA-ME.md
 */
#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void action_ir_principal(lv_event_t * e);
extern void action_ir_alarme(lv_event_t * e);
extern void action_ir_dtc(lv_event_t * e);
extern void action_ir_enlace(lv_event_t * e);
extern void action_ir_config(lv_event_t * e);
extern void action_limiar_menos(lv_event_t * e);
extern void action_limiar_mais(lv_event_t * e);
extern void action_histerese_menos(lv_event_t * e);
extern void action_histerese_mais(lv_event_t * e);
extern void action_janela_menos(lv_event_t * e);
extern void action_janela_mais(lv_event_t * e);
extern void action_salvar_alarme(lv_event_t * e);
extern void action_can_250k(lv_event_t * e);
extern void action_can_500k(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/
