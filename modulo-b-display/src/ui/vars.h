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
    FLOW_GLOBAL_VARIABLE_RPM_TXT = 0,
    FLOW_GLOBAL_VARIABLE_VEL_TXT = 1,
    FLOW_GLOBAL_VARIABLE_TEMP_TXT = 2
};

// Native global variables

extern const char *get_var_rpm_txt();
extern void set_var_rpm_txt(const char *value);
extern const char *get_var_vel_txt();
extern void set_var_vel_txt(const char *value);
extern const char *get_var_temp_txt();
extern void set_var_temp_txt(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/