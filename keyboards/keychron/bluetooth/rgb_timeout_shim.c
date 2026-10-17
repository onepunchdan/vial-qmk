/* Runtime RGB idle-off timer. Keychron's QMK core provides
 * rgb_matrix_disable_timeout_set()/rgb_matrix_disable_time_reset(); vial-qmk core
 * does not, so they live here. Called from housekeeping_task_kb (bluetooth_main.c). */

#include "quantum.h"

#ifdef RGB_MATRIX_ENABLE

static uint32_t rgb_timeout      = RGB_MATRIX_TIMEOUT_INFINITE;
static bool     rgb_timeout_held = false; // true only while this shim turned RGB off

void rgb_matrix_disable_timeout_set(uint32_t timeout) {
    rgb_timeout = timeout;
}

void rgb_matrix_disable_time_reset(void) {
    void last_matrix_activity_trigger(void); // non-static in quantum/keyboard.c, no header
    last_matrix_activity_trigger();
}

void rgb_timeout_task(void) {
    bool timed_out = rgb_timeout != RGB_MATRIX_TIMEOUT_INFINITE && last_input_activity_elapsed() > rgb_timeout;
    if (timed_out) {
        // re-blank every pass: other code (battery/caps indicators) may re-enable RGB meanwhile
        if (rgb_matrix_is_enabled()) {
            rgb_matrix_disable_noeeprom();
            rgb_timeout_held = true;
        }
    } else if (rgb_timeout_held) {
        rgb_matrix_enable_noeeprom();
        rgb_timeout_held = false;
    }
}

#else
void rgb_timeout_task(void) {}
#endif
