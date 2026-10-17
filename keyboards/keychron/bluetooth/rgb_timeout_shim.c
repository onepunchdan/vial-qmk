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

#ifndef RGB_DARK_OFF_DELAY
// Idle time before brightness 0 counts as "off"; longer than lumino's 500 ms fade-in.
#    define RGB_DARK_OFF_DELAY 1000
#endif

void rgb_timeout_task(void) {
    static bool dark_held = false; // RGB turned off because brightness was 0
    uint32_t    idle      = last_input_activity_elapsed();
    bool        timed_out = rgb_timeout != RGB_MATRIX_TIMEOUT_INFINITE && idle > rgb_timeout;
    // Brightness faded to 0 (e.g. by lumino) but RGB still enabled: turn it off so the LED
    // driver shuts down and LPM can sleep. Restored on the next keypress like a timeout.
    bool dark = rgb_matrix_is_enabled() && rgb_matrix_get_val() == 0 && idle > RGB_DARK_OFF_DELAY;
    if (timed_out) {
        // re-blank every pass: other code (battery/caps indicators) may re-enable RGB meanwhile
        if (rgb_matrix_is_enabled()) {
            rgb_matrix_disable_noeeprom();
            rgb_timeout_held = true;
        }
    } else if (dark) {
        rgb_matrix_disable_noeeprom();
        rgb_timeout_held = true;
        dark_held        = true;
    } else if (rgb_timeout_held && (!dark_held || idle <= RGB_DARK_OFF_DELAY)) {
        rgb_matrix_enable_noeeprom();
        rgb_timeout_held = false;
        dark_held        = false;
    }
}

#else
void rgb_timeout_task(void) {}
#endif
