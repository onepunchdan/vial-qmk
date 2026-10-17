/* Copyright 2022 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include "vil_defaults.h"
#ifdef QMK_SETTINGS
#    include "qmk_settings.h"
#endif

socd_cleaner_t socd_opposing_pairs[] = {
  {{KC_W, KC_S}, SOCD_CLEANER_LAST},
  {{KC_A, KC_D}, SOCD_CLEANER_LAST},
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = VIL_KEYMAPS;

// Runs at the end of dynamic_keymap_reset() (also on reflash); vial_init() reloads the seeded combos/TDs into RAM.
void dynamic_keymap_reset_user(void) {
#ifdef VIAL_TAP_DANCE_ENABLE
    static const vial_tap_dance_entry_t td[] = VIL_TAP_DANCE;
    for (uint8_t i = 0; i < VIL_TAP_DANCE_COUNT && i < VIAL_TAP_DANCE_ENTRIES; ++i) {
        dynamic_keymap_set_tap_dance(i, &td[i]);
    }
#endif
#ifdef VIAL_COMBO_ENABLE
    static const vial_combo_entry_t combos[] = VIL_COMBOS;
    for (uint8_t i = 0; i < VIL_COMBO_COUNT && i < VIAL_COMBO_ENTRIES; ++i) {
        dynamic_keymap_set_combo(i, &combos[i]);
    }
#endif
#ifdef QMK_SETTINGS
    // dynamic_keymap_reset() just reset QMK Settings to defaults; seed the .vil values (a 4-byte buffer fits every setting width).
    static const uint16_t qs[][2] = VIL_QMK_SETTINGS;
    for (uint8_t i = 0; i < VIL_QMK_SETTINGS_COUNT; ++i) {
        uint32_t v = qs[i][1];
        qmk_settings_set(qs[i][0], &v, sizeof(v));
    }
#endif
#ifdef VIAL_ENABLE
    vial_init();
#endif
}


#ifdef CAPS_LOCK_LED_INDEX
bool rgb_matrix_indicators_user(void) {
    if (host_keyboard_led_state().caps_lock) {
        rgb_matrix_set_color(CAPS_LOCK_LED_INDEX, 255, 255, 255);
    } else {
        if (!rgb_matrix_get_flags()) {
            rgb_matrix_set_color(CAPS_LOCK_LED_INDEX, 0, 0, 0);
        }
    }

    return false;
}
#endif

bool get_hold_on_other_key_press(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LT(5, KC_SPC):
            // Do not select the hold action when another key is pressed.
            return false;
        default:
            // Immediately select the hold action when another key is pressed.
            return true;
    }
}
