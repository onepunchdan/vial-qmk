# Keychron Q1 Pro

![Keychron Q1 Pro](https://cdn.shopify.com/s/files/1/0059/0630/1017/t/5/assets/keychronq1proqmkviacustommechanicalkeyboard--edited-1669962623486.jpg)

A customizable 81 keys TKL keyboard.

* Keyboard Maintainer: [Keychron](https://github.com/keychron)
* Hardware Supported: Keychron Q1 Pro
* Hardware Availability: [Keychron Q1 Pro Wireless, Customizeable, QMK/VIA Mechanical Keyboard](https://www.keychron.com/products/keychron-k8-pro-qmk-via-wireless-mechanical-keyboard)

Make example for this keyboard (after setting up your build environment):

    make keychron/q1_pro/ansi_knob:default

Flashing example for this keyboard:

    make keychron/q1_pro/ansi_knob:default:flash

**Reset Key**: Connect the USB cable, toggle mode switch to "Off", hold down the *Esc* key or reset button underneath space bar, then toggle then switch to "Cable".

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Before / after first flash

* Keep the Mac/Win DIP switch in the position that selects layer 0 (`dip_switch_update_kb` in `q1_pro.c` sets default layer 2 when the switch is active and layer 0 otherwise). In this keymap layer 2 is a modifier layer whose Backspace position is `QK_BOOT`.
* Clear EEPROM once after first flashing over stock Keychron firmware (`QK_CLEAR_EEPROM`, or hold *Esc* while plugging in the USB cable).
* Every reflash of a new build resets the keymap, tap dances, combos, macros and QMK Settings to the built-in defaults, so save your Vial layout to a `.vil` file first.

Imported from Keychron/qmk_firmware branch `wls_2025q1`, commit `c8b2b237b4ef` (keyboards/keychron/q1_pro and keyboards/keychron/bluetooth). Local changes: Vial port (no factory test, no raw HID 0xAA/0xAB, common/matrix.c folded into matrix.c, LSI enabled, encoder and RGB timeout shims).

Clearing Bluetooth pairings on the CKBT51 module and updating its firmware require Keychron's stock firmware (factory test / module DFU not ported).
