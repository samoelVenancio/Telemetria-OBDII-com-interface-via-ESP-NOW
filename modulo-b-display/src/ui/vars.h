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
    FLOW_GLOBAL_VARIABLE_VALUE_1 = 0,
    FLOW_GLOBAL_VARIABLE_VALUE_2 = 1,
    FLOW_GLOBAL_VARIABLE_VALUE_3 = 2,
    FLOW_GLOBAL_VARIABLE_VALUE_4 = 3,
    FLOW_GLOBAL_VARIABLE_VALUE_5 = 4,
    FLOW_GLOBAL_VARIABLE_VALUE_6 = 5,
    FLOW_GLOBAL_VARIABLE_STATUS_ESPNOW = 6,
    FLOW_GLOBAL_VARIABLE_REV_SYS = 7,
    FLOW_GLOBAL_VARIABLE_MSG_STATUS = 8,
    FLOW_GLOBAL_VARIABLE_TEMP_MAX_ALARM = 9,
    FLOW_GLOBAL_VARIABLE_ECO_MODE_RPM = 10
};

// Native global variables

extern const char *get_var_value_1();
extern void set_var_value_1(const char *value);
extern const char *get_var_value_2();
extern void set_var_value_2(const char *value);
extern const char *get_var_value_3();
extern void set_var_value_3(const char *value);
extern const char *get_var_value_4();
extern void set_var_value_4(const char *value);
extern const char *get_var_value_5();
extern void set_var_value_5(const char *value);
extern const char *get_var_value_6();
extern void set_var_value_6(const char *value);
extern const char *get_var_status_espnow();
extern void set_var_status_espnow(const char *value);
extern const char *get_var_rev_sys();
extern void set_var_rev_sys(const char *value);
extern const char *get_var_msg_status();
extern void set_var_msg_status(const char *value);
extern const char *get_var_temp_max_alarm();
extern void set_var_temp_max_alarm(const char *value);
extern const char *get_var_eco_mode_rpm();
extern void set_var_eco_mode_rpm(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/