# Air75 V2 Vial keymap

Built-in layout comes from `airv2_layer6.vil` via `util/vil_tool.py` (`vil_defaults.h`). Tap dance, combo and layers are seeded into EEPROM by `dynamic_keymap_reset_user()` in `keymap.c`.

## Custom keycode rename (jincao1 -> ryodeushii)

The `.vil` was saved against the old jincao1 firmware, where custom keycodes counted from `QK_KB_0` (`USER00`) in this order. The new firmware uses ryodeushii's enum (`keyboards/nuphy/common/core/keys.h`), so the used ones are renamed when generating `vil_defaults.h`:

| .vil | jincao1 name | ryodeushii name |
|---|---|---|
| USER01 | LNK_USB | LNK_USB |
| USER02 | LNK_RF | LNK_RF |
| USER03 | LNK_BLE1 | LNK_BLE1 |
| USER04 | LNK_BLE2 | LNK_BLE2 |
| USER05 | LNK_BLE3 | LNK_BLE3 |
| USER08 | SIDE_VAI | SIDE_VAI |
| USER09 | SIDE_VAD | SIDE_VAD |
| USER10 | SIDE_MOD | SIDE_MOD |
| USER11 | SIDE_HUI | SIDE_HUI |

The names match but the numeric values differ (e.g. `SIDE_VAI` was `QK_KB_8`, now `QK_KB_17`), so a layout saved from the old firmware must not be restored onto this one. Regenerate with:

    python3 util/vil_tool.py keymap airv2_layer6.vil nuphy/air75v2/ansi vil_defaults.h --rename USER01=LNK_USB --rename USER02=LNK_RF --rename USER03=LNK_BLE1 --rename USER04=LNK_BLE2 --rename USER05=LNK_BLE3 --rename USER08=SIDE_VAI --rename USER09=SIDE_VAD --rename USER10=SIDE_MOD --rename USER11=SIDE_HUI

`vial.json` `customKeycodes` lists all 45 ryodeushii keycodes in enum order from `QK_KB_0`. Vial unlock combo: Esc (0,0) + Enter (3,13).

## Memory notes

- Emulated EEPROM is 2048 B on this board (`FEE_DENSITY_BYTES` in `air75v2/ansi/config.h`), set so Vial fits the F072's 16 KB RAM. That leaves 304 B for Vial macros, far less than on the other boards.
- After first flashing this firmware over the old jincao1-based firmware, clear EEPROM once (`QK_CLEAR_EEPROM` / `EE_CLR`, or bootmagic: hold Esc while plugging in), because the emulated-EEPROM layout changed.
