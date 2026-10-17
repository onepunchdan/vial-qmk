# Vial upstream sync + RK61 / Air75 v2 / Q1 Pro ports Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Sync the fork with `vial-kb/vial`, replace the RK61 and Air75 v2 with ports from maintained sources, add a Bluetooth-capable Q1 Pro ANSI knob port, and give every board the same baseline and built-in keymaps from the user's `.vil` files.

**Architecture:** Board ports are folder imports from pinned upstream commits plus board-local shims for API drift. The only core change is Keychron's opt-in RGB driver shutdown/load code, behind `RGB_MATRIX_DRIVER_SHUTDOWN_ENABLE` / `RGB_MATRIX_DRIVER_LOAD_ENABLE`. A small stdlib-Python tool converts `.vil` layouts into a generated header per keymap and checks `vial.json` against the board matrix.

**Tech Stack:** QMK/vial-qmk (ChibiOS, arm-none-eabi-gcc), `qmk` CLI, Python 3 stdlib (`json`, `unittest`).

**Spec:** `docs/superpowers/specs/2026-10-16-vial-wireless-ports-design.md`

## Global Constraints

- Repo `/home/dan/repos/vial-qmk`, work branch `feat/wireless-ports` off `vial`. `git -C /home/dan/repos/vial-qmk` on every git call.
- Subagents never run `git add`, `commit`, `push`, `stash`, `merge`, `checkout` of branches, or write outside the repo and the scratchpad. Commit steps below are main-thread, after user approval.
- Escalate (stop and report) any decision this plan and the spec do not settle that changes behaviour.
- Scratchpad: `S=/tmp/claude-1000/-home-dan-repos-vial-qmk/ad268a08-51c2-4299-8fa3-640c2a04567b/scratchpad`. Source clones: `$S/src/{keychron,ryo,vial}.git` (bare, blobless). Never build inside downloaded trees.
- Pinned sources: Keychron `wls_2025q1` = `c8b2b237b4ef`; ryodeushii `nuphy-keyboards` = `5ffb425f73d2`; QMK master for `royal_kludge/rk61` = `180f394b489c` (board added in `02cf2b56fd`); vial upstream `vial` = `dd43959ae5`.
- `.vil` sources (read-only, copy only): `/home/dan/Sync/popos/modern-space-cadet/qmk/vial_2026/{airv2,q3,q8,rk61,v3,v4,v8}_layer6.vil`.
- Every board: `DYNAMIC_KEYMAP_LAYER_COUNT 6`; `VIAL_TAP_DANCE_ENTRIES 32`, `VIAL_COMBO_ENTRIES 32` (current builds already get 32/32; if any board fails the EEPROM static assert, step down 16, 8, 4 to the highest tier all boards build with, floor 4 TD / 2 combo, same tier everywhere, and report); `VIAL_KEY_OVERRIDE_ENTRIES 0`; `MOUSEKEY_ENABLE = yes`.
- Keymap baseline (`config.h`, `rules.mk`, `keymap.json`, `rgb_matrix_user.inc`, SOCD pairs, `get_hold_on_other_key_press`): copy verbatim from `keyboards/keychron/q3/ansi_encoder/keymaps/vial/` as described in the spec's "Shared baseline"; `COMMAND_ENABLE = yes` on RK61, Air75 v2, Q1 Pro; `no` on Q3/Q8/V3/V4/V8 (as today).
- Board RGB animations: `solid_color`, `cycle_left_right`, all `solid_reactive*` on; rest off.
- Keep `quantum/qmk_settings.c` fork edit, the 4 module submodules, existing Vial UIDs for Q3/Q8/V3/V4/V8/RK61. Fresh UIDs (`python3 util/vial_generate_keyboard_uid.py`) for Air75 v2 and Q1 Pro.
- Do not modify `keyboards/keychron/common/`.

## Review Focus

1. Fresh flash on any board (EEPROM reset): the user expects layers, the 4 tap dances and the combo from their `.vil` to be present without restoring. Pinned by Task 3's `eeconfig_init_user` seeding test and Task 9's symbol/round-trip check.
2. Vial app opening each board: `vial.json` key positions must exactly match the board's matrix, or keys show/bind wrong. Pinned by `check-vialjson` in Tasks 2, 4, 5, 8.
3. Q1 Pro BT/battery keys bound in Vial must trigger Bluetooth actions: `vial.json` `customKeycodes` order must equal the `q1_pro.h` enum order. Pinned by Task 8 step comparing names to enum.
4. Q1 Pro asleep on battery must wake and keep time: LSI must be on (`STM32_LSI_ENABLED TRUE`). Pinned by Task 7 preprocessor check.
5. Vial over USB on Air75 v2 / Q1 Pro must not be swallowed by vendor raw-HID handlers (`0xFE` prefix). Pinned by Tasks 5 and 7 grep/inspection steps.

---

### Task 0: Branch and baseline measurements

**Files:** none in repo. Output: `$S/baseline-sizes.txt`.

- [ ] **Step 1 (main thread):** `git -C /home/dan/repos/vial-qmk switch -c feat/wireless-ports vial`
- [ ] **Step 2:** Build current set: `./compkeebs.sh` from repo root. Expected: all 7 targets `[OK]`.
- [ ] **Step 3:** Record sizes: `for f in .build/*_vial.elf; do arm-none-eabi-size $f | tail -1; done > $S/baseline-sizes.txt`. Expected: 7 lines.

### Task 1: Upstream sync (main thread)

- [ ] **Step 1:** `git -C /home/dan/repos/vial-qmk fetch https://github.com/vial-kb/vial-qmk.git vial` then `git -C /home/dan/repos/vial-qmk merge --no-commit --no-ff FETCH_HEAD`. Expected: no conflicts (12 keyboard-only commits).
- [ ] **Step 2:** `git -C /home/dan/repos/vial-qmk submodule update --init --recursive`; rerun `./compkeebs.sh`. Expected: all `[OK]`.
- [ ] **Step 3:** After user approval: `git -C /home/dan/repos/vial-qmk commit -m "Merge vial-kb/vial dd43959ae5"`.

### Task 2: `.vil` conversion and `vial.json` check tool

**Files:**
- Create: `util/vil_tool.py`
- Test: `util/test_vil_tool.py`

**Interfaces:**
- Produces CLI:
  - `python3 util/vil_tool.py keymap <file.vil> <keyboard_name> <out.h> [--rename OLD=NEW ...] [--layout-macro LAYOUT_xxx]`: writes a header defining `VIL_KEYMAPS` (initializer for `keymaps[][MATRIX_ROWS][MATRIX_COLS]` in matrix order, one `{...}` per layer), `VIL_ENCODER_MAP` (initializer for `encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS]`, only if `encoder_layout` present), `VIL_TAP_DANCE` (array initializer of `vial_tap_dance_entry_t`, non-empty entries only, with index order kept), `VIL_COMBOS` (`vial_combo_entry_t`, same rule), and `VIL_TAP_DANCE_COUNT`, `VIL_COMBO_COUNT`.
  - `python3 util/vil_tool.py check-vialjson <vial.json> <keyboard_name>`: exit 0 if the set of `"r,c"` legends in `vial.json` `layouts.keymap` (excluding encoder `e` entries) equals the set of matrix positions in `qmk info -kb <keyboard_name> -f json` layout, else print the diff and exit 1.
- Python: `convert_keycode(name: str, renames: dict[str, str]) -> str`; `vil_to_header(vil: dict, renames: dict[str, str]) -> str`; `vialjson_positions(vialjson: dict) -> set[tuple[int, int]]`.

`.vil` facts: `layout[layer][row][col]` is already matrix-ordered, `-1` = no key position; `tap_dance` entries are `[on_tap, on_hold, on_double_tap, on_tap_hold, term]`; `combo` entries are `[k1, k2, k3, k4, output]`; `encoder_layout[layer][enc] = [ccw, cw]`. Keycode strings use Vial GUI names (old QMK names).

- [ ] **Step 1: Write failing tests** in `util/test_vil_tool.py` (unittest):
  - `test_lt_layer_form`: `convert_keycode("LT5(KC_SPACE)", {}) == "LT(5, KC_SPACE)"`.
  - `test_user_to_qk_kb`: `convert_keycode("USER03", {}) == "QK_KB_3"`; `convert_keycode("USER03", {"USER03": "LINK_BT1"}) == "LINK_BT1"`.
  - `test_legacy_names`: `KC_LSHIFT -> KC_LSFT`, `KC_PSCREEN -> KC_PSCR`, `KC_SCROLLLOCK -> KC_SCRL`, `KC_LCTRL -> KC_LCTL`, `KC_RSHIFT -> KC_RSFT`, `KC_APPLICATION -> KC_APP`.
  - `test_hex_and_passthrough`: `"0x7e05" -> "0x7e05"`; `"TD(2)" -> "TD(2)"`; `"QK_CAPS_WORD_TOGGLE"` unchanged; `-1` -> `"KC_NO"`.
  - `test_header_from_q3_vil`: load a copy of `q3_layer6.vil`, header contains `VIL_TAP_DANCE_COUNT 4`, `VIL_COMBO_COUNT 1`, exactly 6 layer initializers, and `LT(5, KC_SPACE)`.
  - `test_vialjson_positions`: a 2-key KLE `[["0,0","0,1"],["0,0\n\n\n\n\n\n\n\n\ne"]]` yields `{(0,0),(0,1)}`.
- [ ] **Step 2:** `python3 -m unittest util/test_vil_tool.py -v`. Expected: FAIL (module missing).
- [ ] **Step 3:** Implement `util/vil_tool.py`. Legacy-name table: start with the pairs in the test, extend only as compile errors demand (each addition gets a test line).
- [ ] **Step 4:** Rerun tests. Expected: all PASS.
- [ ] **Step 5:** Round-trip sanity: `python3 util/vil_tool.py check-vialjson keyboards/keychron/q3/ansi_encoder/keymaps/vial/vial.json keychron/q3/ansi_encoder`. Expected: exit 0.

### Task 3: Baseline on existing Keychron boards (Q3, Q8, V3, V4, V8)

**Files (each of `q3/ansi_encoder`, `q8/ansi_encoder`, `v3/ansi_encoder`, `v4/ansi`, `v8/ansi_encoder` under `keyboards/keychron/`):**
- Create: `keymaps/vial/vil_defaults.h` (generated)
- Modify: `keymaps/vial/config.h`, `keymaps/vial/rules.mk`, `keymaps/vial/keymap.c`

**Interfaces:**
- Consumes: `util/vil_tool.py keymap` (Task 2).
- Produces: keymap pattern reused by Tasks 4, 5, 8:
  - `keymap.c` uses `const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = VIL_KEYMAPS;` (and `encoder_map = VIL_ENCODER_MAP` where present). Layer enums stay for readability only.
  - `void eeconfig_init_user(void)` writes `VIL_TAP_DANCE[i]` via `dynamic_keymap_set_tap_dance(i, &entry)` and `VIL_COMBOS[i]` via `dynamic_keymap_set_combo(i, &entry)` (both declared in `quantum/dynamic_keymap.h`, return `int`). It runs after `dynamic_keymap_reset()` in `eeconfig_init_quantum()` (`quantum/eeconfig.c:157-160`), so seeds survive the reset.

- [ ] **Step 1:** Generate headers, e.g. `python3 util/vil_tool.py keymap $VIL/q3_layer6.vil keychron/q3/ansi_encoder keyboards/keychron/q3/ansi_encoder/keymaps/vial/vil_defaults.h` (V4 uses `v4_layer6.vil`, etc.).
- [ ] **Step 2:** Edit keymaps per Interfaces; add to `config.h`: `VIAL_TAP_DANCE_ENTRIES 32`, `VIAL_COMBO_ENTRIES 32`, `VIAL_KEY_OVERRIDE_ENTRIES 0`; add `MOUSEKEY_ENABLE = yes` to `rules.mk`. Leave `process_record_user` / `housekeeping_task_user` Keychron calls as they are.
- [ ] **Step 3:** Build all five: `qmk compile -c -kb <kb> -km vial`. Expected: `[OK]`. If the EEPROM static assert in `quantum/nvm/eeprom/nvm_dynamic_keymap.c:103` fires, escalate with numbers (spec tier rule).
- [ ] **Step 4:** Seed check: `arm-none-eabi-nm .build/keychron_q3_ansi_encoder_vial.elf | grep -c eeconfig_init_user` returns 1, and `git -C /home/dan/repos/vial-qmk diff --stat -- keyboards/keychron/common` is empty.
- [ ] **Step 5:** Report `.bin` sizes vs `$S/baseline-sizes.txt`.
- [ ] **Step 6 (main thread, after approval):** commit `feat(keychron): 32 TD/32 combo, mouse keys, built-in keymaps from layer6 .vil`, including `util/vil_tool.py` and its test.

### Task 4: RK61 → `royal_kludge/rk61`

**Files:**
- Create: `keyboards/royal_kludge/rk61/` (from QMK `180f394b489c`), `keyboards/royal_kludge/rk61/keymaps/vial/{config.h,rules.mk,keymap.c,keymap.json,rgb_matrix_user.inc,vial.json,vil_defaults.h}`
- Delete: `keyboards/hypharf/`

- [ ] **Step 1:** `git -C $S/src/keychron.git archive 180f394b489c keyboards/royal_kludge/rk61 | tar -x -C /home/dan/repos/vial-qmk`.
- [ ] **Step 2:** Board edits: `config.h` add `DYNAMIC_KEYMAP_LAYER_COUNT 6`; `keyboard.json` features add `mousekey: true`, `command: true`, `deferred_exec` not needed; RGB animations per Global Constraints. Fix any schema errors vial-qmk raises.
- [ ] **Step 3:** Keymap: copy baseline files from `keyboards/hypharf/rk61/keymaps/vial/` (keep its `VIAL_KEYBOARD_UID {0xAF, 0x0F, 0x41, 0x84, 0x93, 0xA9, 0x2E, 0xC8}` and `vial.json`), apply Task 3 pattern with `rk61_layer6.vil`. Set `VIAL_UNLOCK_COMBO_ROWS { 0, 2 }` / `VIAL_UNLOCK_COMBO_COLS { 0, 12 }` (Esc + Enter; confirm Enter is matrix `2,12` in `keyboard.json`, else use its real position).
- [ ] **Step 4:** `qmk compile -c -kb royal_kludge/rk61 -km vial`. Expected `[OK]`. `python3 util/vil_tool.py check-vialjson keyboards/royal_kludge/rk61/keymaps/vial/vial.json royal_kludge/rk61`. Expected exit 0.
- [ ] **Step 5:** `arm-none-eabi-size .build/royal_kludge_rk61_vial.elf`: report flash and data+bss vs baseline `hypharf_rk61_vial`.
- [ ] **Step 6:** `rm -r keyboards/hypharf` (working tree only; staging happens at commit time on the main thread).
- [ ] **Step 7 (main thread, after approval):** commit `feat(rk61): port to royal_kludge/rk61 with vial keymap`.

### Task 5: Air75 v2 → ryodeushii `nuphy/air75v2`

**Files:**
- Create: `keyboards/nuphy/rules.mk`, `keyboards/nuphy/common/`, `keyboards/nuphy/air75v2/` (ansi only), `keyboards/nuphy/air75v2/ansi/keymaps/vial/*`
- Modify: `keyboards/nuphy/common/core/debounce.c` (signature shim)
- Delete: `keyboards/nuphy/air75_v2/`, `keyboards/nuphy/air75v2/ansi/keymaps/{ryodeushii,srgb}/`; non-air75v2 boards are not imported.

**Interfaces:**
- Consumes: Task 2 tool with `--rename`; Task 3 keymap pattern.
- Produces: jincao1→ryodeushii keycode rename table (documented in `keymaps/vial/readme.md`).

- [ ] **Step 1:** `git -C $S/src/ryo.git archive 5ffb425f73d2 keyboards/nuphy/rules.mk keyboards/nuphy/common keyboards/nuphy/air75v2 | tar -x -C /home/dan/repos/vial-qmk`; delete the `ryodeushii` and `srgb` keymaps; add `Source: ryodeushii/qmk-firmware@5ffb425f73d2` to `air75v2/ansi/readme.md`.
- [ ] **Step 2:** Shim `common/core/debounce.c`: `void debounce_init(uint8_t num_rows)` and `bool debounce(matrix_row_t raw[], matrix_row_t cooked[], uint8_t num_rows, bool changed)` (args unused). Check `keyboards/nuphy/rules.mk` does not pull in gem80/halo-only files.
- [ ] **Step 3:** `qmk compile -c -kb nuphy/air75v2/ansi -km default`. Expected `[OK]`, ~67.6 KB flash.
- [ ] **Step 4:** Rename table: map each `USERnn` used in `airv2_layer6.vil` (`USER01..05`, `USER08..11`) through the jincao1 enum in the old `keyboards/nuphy/air75_v2/ansi/user_kb.h` (read before deleting) to the ryodeushii name in `keyboards/nuphy/common/core/keys.h`. Any without an equivalent: escalate.
- [ ] **Step 5:** Keymap: baseline files, fresh UID, `--rename` table into `vil_defaults.h`, Task 3 pattern; board `config.h` 6 layers; `keyboard.json` `mousekey: true`, `command: true`, RGB animations per Global Constraints. Unlock combo Esc + Enter at their real matrix positions.
- [ ] **Step 6:** `vial.json`: from `air75v2/ansi/keymaps/via/NuPhy Air75 V2 via3.json` layout, `lighting: vialrgb`, `customKeycodes` listing ryodeushii's enum in order from `QK_KB_0` (all 45 if the Vial GUI accepts more than 32; check vial-gui source `src/main/python/keycodes/keycodes.py` for the custom keycode limit; if limited to 32, escalate).
- [ ] **Step 7:** Raw HID: read `common/config/via.c`; confirm it hooks only `via_custom_value_command_kb` (or IDs other than `0xFE`) and cannot consume `id_vial_prefix`. Report the hook names.
- [ ] **Step 8:** `qmk compile -c -kb nuphy/air75v2/ansi -km vial` `[OK]`; `check-vialjson` exit 0; report flash and data+bss vs baseline `nuphy_air75_v2_ansi_vial`; `arm-none-eabi-nm` shows `vial_handle_cmd` and `housekeeping_task_nuphy`.
- [ ] **Step 9:** `rm -r keyboards/nuphy/air75_v2`.
- [ ] **Step 10 (main thread, after approval):** commit `feat(nuphy): port air75v2 from ryodeushii with vial keymap`.

### Task 6: Keychron RGB driver shutdown/load core support

**Files:**
- Modify: `quantum/rgb_matrix/rgb_matrix.{c,h}`, `quantum/rgb_matrix/rgb_matrix_drivers.{c,h}`, `drivers/led/snled27351.{c,h}`

**Interfaces:**
- Produces (only when the flag is defined): `bool rgb_matrix_driver_allow_shutdown(void)` (weak, default true), `bool rgb_matrix_is_driver_shutdown(void)`, `rgb_matrix_driver.shutdown/exit_shutdown` under `RGB_MATRIX_DRIVER_SHUTDOWN_ENABLE`; `uint8_t (*get_load_ratio)(void)` in `rgb_matrix_driver_t` and `snled27351_get_load_ratio()` under `RGB_MATRIX_DRIVER_LOAD_ENABLE`.

- [ ] **Step 1:** Extract Keychron's hunks: `MB=$(git -C $S/src/keychron.git merge-base c8b2b237b4ef refs/qmk/master); git -C $S/src/keychron.git diff $MB c8b2b237b4ef -- quantum/rgb_matrix drivers/led/snled27351.c drivers/led/snled27351.h > $S/kc-rgb.patch`.
- [ ] **Step 2:** Apply only the hunks for the two flags (skip `RGB_MATRIX_TIMEOUT`, `RGB_MATRIX_BRIGHTNESS_TURN_OFF_VAL`, led_matrix, spi/mono variants). Wrap Keychron's unguarded `snled27351_*` shutdown/load functions in the matching flag.
- [ ] **Step 3:** No-op proof: rebuild `keychron/q3/ansi_encoder:vial` and `planck/rev6:default`; `.bin` byte-identical to the Task 3 / pre-change build (`cmp`). Expected: identical.
- [ ] **Step 4 (main thread, after approval):** commit `feat(core): opt-in RGB matrix driver shutdown/load hooks from Keychron`.

### Task 7: Q1 Pro board + Bluetooth stack

**Files:**
- Create: `keyboards/keychron/bluetooth/` (whole dir), `keyboards/keychron/q1_pro/` (shared files + `ansi_knob/`; no `iso_knob`, `jis_encoder`, `via_json`)
- Modify (board-local only): `q1_pro/rules.mk`, `q1_pro/mcuconf.h`, `q1_pro/q1_pro.c`, `q1_pro/matrix.c`

- [ ] **Step 1:** Remove empty leftover dirs `keyboards/keychron/{q1_pro,bluetooth}`; `git -C $S/src/keychron.git archive c8b2b237b4ef keyboards/keychron/bluetooth keyboards/keychron/q1_pro | tar -x -C /home/dan/repos/vial-qmk`; drop non-ansi variants; note source hash in `q1_pro/readme.md`.
- [ ] **Step 2:** Matrix: replace `q1_pro/matrix.c` with Keychron's `common/matrix.c` content (`git -C $S/src/keychron.git show c8b2b237b4ef:keyboards/keychron/common/matrix.c`); in `q1_pro/rules.mk` replace `include keyboards/keychron/common/common.mk` with `SRC += matrix.c` and keep `CUSTOM_MATRIX` setting as Keychron's `info.json`/rules require. Remove `-DENABLE_FACTORY_TEST` and `bluetooth/factory_test.c` from the build.
- [ ] **Step 3:** `q1_pro/mcuconf.h`: `#undef STM32_LSI_ENABLED` / `#define STM32_LSI_ENABLED TRUE`. Check: `grep -n STM32_LSI_ENABLED keyboards/keychron/q1_pro/mcuconf.h` shows TRUE.
- [ ] **Step 4:** Encoder shim in `q1_pro.c`: replace `encoder_quadrature_handle_inerrupt_read` with vial-qmk's `encoder_handle_read()` equivalent (read `drivers/encoder/encoder_quadrature.c` for the exact name/signature); no edits under `drivers/encoder`.
- [ ] **Step 5:** Raw HID: delete `via_command_kb` and its `!VIA_ENABLE` `raw_hid_receive` wrapper from `q1_pro.c` (both cases, `0xAA` and `0xAB`, are not ported per spec). Check: `grep -rn "raw_hid_receive\|via_command_kb" keyboards/keychron/q1_pro keyboards/keychron/bluetooth` shows no handler that could see `0xFE`.
- [ ] **Step 6:** `qmk compile -c -kb keychron/q1_pro/ansi_knob -km default`. Fix board-level API drift (data-driven keys, `snled27351` config names: compare with `keyboards/keychron/q3`). Any fix needing core outside Task 6: escalate.
- [ ] **Step 7:** `arm-none-eabi-nm .build/keychron_q1_pro_ansi_knob_default.elf | grep -E "bluetooth_task|ckbt51_|lpm_task|rgb_matrix_driver_allow_shutdown"`. Expected: each present.

### Task 8: Q1 Pro Vial keymap

**Files:**
- Create: `keyboards/keychron/q1_pro/ansi_knob/keymaps/vial/{config.h,rules.mk,keymap.c,keymap.json,rgb_matrix_user.inc,vial.json,vil_defaults.h}`, `$S/q1pro_layer6.vil` (derived, scratch only)

**Interfaces:**
- Consumes: Task 2 tool; Task 3 pattern.

- [ ] **Step 1:** Derive `$S/q1pro_layer6.vil`: key identity = stock layer-0 keycode. For each Q1 Pro matrix position, find the Q3 position whose stock layer-0 keycode (Q3 `keymaps/default/keymap.c`) equals the Q1 Pro stock layer-0 keycode (`q1_pro/ansi_knob/keymaps/default/keymap.c`), and copy that Q3 position's 6 layers from `q3_layer6.vil`. Unmatched Q1 positions keep the Q1 Pro default keycode on layer 0 and `KC_TRNS` above. Copy `tap_dance`, `combo`, `encoder_layout` from `q3_layer6.vil`. Print the unmatched list in the report. Q1 Pro BT keys (`BT_HST1..3`, `BAT_LVL`): keep them where the Q1 Pro default keymap's Fn layer has them, on the same layer index in the derived map.
- [ ] **Step 2:** Keymap files: baseline, fresh UID, `ENCODER_MAP_ENABLE = yes`, `VIAL_ENCODERS_ENABLE` per vial-qmk docs, `MOUSEKEY_ENABLE = yes`, `COMMAND_ENABLE = yes`; unlock Esc + Enter at real positions; Task 3 pattern with `$S/q1pro_layer6.vil`; board `config.h` 6 layers; RGB animations per Global Constraints.
- [ ] **Step 3:** `vial.json` from Keychron's VIA json for q1_pro ansi_knob (`git -C $S/src/keychron.git show c8b2b237b4ef:keyboards/keychron/q1_pro/via_json/<ansi knob file>`), knob as Vial encoder entries, `lighting: vialrgb`, `customKeycodes` = the 13 names of the `q1_pro.h` enum in order (`KC_LOPTN … KC_SIRI, BT_HST1, BT_HST2, BT_HST3, BAT_LVL`).
- [ ] **Step 4:** Order check: script compares `customKeycodes[i].name`-to-enum mapping list you wrote against `q1_pro.h` enum order; index of `BT_HST1` must be 9, `BAT_LVL` 12.
- [ ] **Step 5:** `qmk compile -c -kb keychron/q1_pro/ansi_knob -km vial` `[OK]`; `check-vialjson` exit 0; nm shows `vial_handle_cmd`, `bluetooth_task`, `lpm_task`, `eeconfig_init_user`; report size.
- [ ] **Step 6 (main thread, after approval):** commit Tasks 7+8 together `feat(keychron): q1_pro ansi_knob with CKBT51 bluetooth and vial keymap`.

### Task 9: Build script and whole-branch verification

**Files:**
- Modify: `compkeebs.sh`

- [ ] **Step 1:** `compkeebs.sh` targets: `royal_kludge/rk61`, `keychron/v4/ansi`, `keychron/v3/ansi_encoder`, `keychron/v8/ansi_encoder`, `keychron/q3/ansi_encoder`, `keychron/q8/ansi_encoder`, `nuphy/air75v2/ansi`, `keychron/q1_pro/ansi_knob`, all `-km vial`.
- [ ] **Step 2:** Run `./compkeebs.sh` and `qmk compile -c -kb planck/rev6 -km default`. Expected: all `[OK]`.
- [ ] **Step 3:** Round-trip each built-in keymap: for each board, `python3 util/vil_tool.py keymap <its .vil> ...` regenerated to `$S/` and `diff` against the committed `vil_defaults.h`. Expected: no diff.
- [ ] **Step 4:** Size table (flash, data+bss) for all 8 boards vs `$S/baseline-sizes.txt`.
- [ ] **Step 5:** Reviewer (fresh agent) judges the branch against the spec, including a separate check that no vendor code intercepts `0xFE`.
- [ ] **Step 6 (main thread, after approval):** commit `chore: update compkeebs.sh for new board paths`. Push only on explicit user request.
