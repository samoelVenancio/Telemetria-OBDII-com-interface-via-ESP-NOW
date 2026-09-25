/**
 * ui_ponte.h — Ponte entre os dados do sistema (estado, alarme, NVS, enlace)
 * e a interface gerada pelo EEZ Studio.
 *
 * O EEZ gera as telas e, para cada "Native global variable" declarada no
 * projeto, chama get_var_<nome>() no ui_tick(). Esta ponte implementa esses
 * getters e as actions dos botões. Contrato de nomes: ../eez/LEIA-ME.md
 */
#pragma once

/* Cria o lv_timer de 10 Hz que atualiza as variáveis e chama ui_tick().
 * CHAMAR COM lvgl_port_lock() SEGURADO, depois de ui_init(). */
void ui_ponte_iniciar(void);
