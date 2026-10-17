/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

//Vial Keyboard UID
#define VIAL_KEYBOARD_UID {0xAC, 0x4E, 0xF7, 0xB3, 0x4E, 0xD0, 0xC0, 0x76}

#define VIAL_UNLOCK_COMBO_ROWS { 0, 3 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 13 }
#define VIALRGB_NO_DIRECT

#define TAPPING_TERM 200
#define PERMISSIVE_HOLD
#define QUICK_TAP_TERM 120
#define SPECULATIVE_HOLD

#ifndef HOLD_ON_OTHER_KEY_PRESS_PER_KEY
#define HOLD_ON_OTHER_KEY_PRESS_PER_KEY
#endif

// #define FLOW_TAP_TERM 150
// #define CAPS_WORD_IDLE_TIMEOUT 3000

#define PALETTEFX_REACTIVE_ENABLE  // only palettefx effect kept
#define RGB_MATRIX_KEYPRESSES      // needed by PaletteFx reactive now that no standard reactive effect is enabled
#define PALETTEFX_ENABLE_ALL_PALETTES

#define VIAL_TAP_DANCE_ENTRIES 16
#define VIAL_COMBO_ENTRIES 16
