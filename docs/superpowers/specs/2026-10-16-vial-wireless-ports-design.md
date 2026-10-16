# Design: upstream sync + proper ports for RK61, Air75 v2, Q1 Pro

Date: 2026-10-16. Repo: `onepunchdan/vial-qmk` (branch `vial`), local at `/home/dan/repos/vial-qmk`.

## Intent

The fork carries Vial firmware for the user's daily keyboards: Keychron Q3, Q8, V3, V4, V8, NuPhy Air75 v2, and an RK61. The Air75 v2 and RK61 were hand-merged ("frankenstein") from outside sources. The user wants:

1. The fork brought up to date with `vial-kb/vial-qmk` (`vial` branch).
2. The RK61 and Air75 v2 replaced with clean ports built from maintained sources.
3. A new Vial port for the Keychron Q1 Pro (ANSI knob) with Bluetooth working on day one.
4. Every board carrying the same personal baseline (below), so all keyboards behave alike.

The origin was a Gemini-written plan (`~/Downloads/vial_qmk_wireless_porting_orchestration_plan.md`). It was reviewed and corrected; this document supersedes it.

## Goals

- `vial-kb/vial` merged into the fork, every board in `compkeebs.sh` still builds.
- `royal_kludge/rk61:vial`, `nuphy/air75v2/ansi:vial`, `keychron/q1_pro/ansi_knob:vial` build cleanly with the shared baseline.
- Q1 Pro: Bluetooth 3-host switching, USB/BT mode switch, battery level, low-power sleep, knob remappable in Vial, BT keys exposed as Vial custom keycodes.
- Air75 v2: wireless (BLE + 2.4 GHz + USB) via the ryodeushii stack, wireless keys exposed as Vial custom keycodes.
- Non-Keychron-wireless boards compile byte-for-byte the same behaviour as before any core change (core changes are opt-in behind flags).

## Non-goals

- Vendor- or fork-specific extras: jincao1 customizations (`customizations.md`), ryodeushii personal keymaps / SignalRGB / custom VIA menus, RK61 manufacturer factory-reset and LED-test combos, Keychron factory test mode beyond what is needed to compile.
- ISO/JIS Q1 Pro variants, other NuPhy boards (gem80, halo), other Keychron wireless boards.
- Merging newer `qmk/qmk_firmware` directly. Upstream for this fork means `vial-kb/vial-qmk` only.
- Wireless on the RK61 (the user's unit is USB-only).

## Anti-goals (failure even if it compiles)

- Fewer than 6 layers on any board.
- Breaking Q3/Q8/V3/V4/V8, or changing them beyond the agreed baseline edits: in particular, overwriting `keyboards/keychron/common/keychron_common.{c,h}`.
- Core QMK changes that affect boards which do not opt in.
- Vial raw HID (`0xFE` prefix) or unlock flow intercepted by vendor code.
- Any commit, push, or flash without the user's approval.

## Constraints

- STM32F072 boards (RK61, Air75 v2): 128 KB flash, 16 KB RAM, emulated EEPROM. Current builds: RK61 ~56 KB flash, Air75 v2 ~65.6 KB; data+bss ~16 KB (includes reserved stacks). RAM is the tight resource, not flash.
- 6 layers is non-negotiable. If space runs out, first trim `PALETTEFX_ENABLE_ALL_EFFECTS` / `PALETTEFX_ENABLE_ALL_PALETTES` to a few palettes; dropping whole modules is the last resort and needs the user's OK.
- vial-qmk's QMK base: QMK master as of 2025-06-21. Sources come from other bases (Keychron 0.28, ryodeushii 0.33.7); API drift must be shimmed in board code, not by changing core.
- Testing on hardware is the user's job. The builder proves build success, size, and symbol presence only.

## Shared baseline (every board's `vial` keymap)

Taken from the existing Q3/V4/Air75 v2/RK61 keymaps; replicate exactly.

- Board `config.h`: `#define DYNAMIC_KEYMAP_LAYER_COUNT 6`.
- Keymap `config.h`: explicit `VIAL_TAP_DANCE_ENTRIES` / `VIAL_COMBO_ENTRIES`, same values on every board (Q3/Q8/V3/V4/V8 included). User's rule: 32 and 32 (what the current RK61/Air75 v2 builds already get from Vial's auto-sizing in `quantum/vial.h`, which keys off total EEPROM size). Fewer slots were only ever a concession to keep 6 layers; if a board fails the EEPROM static assert (`quantum/nvm/eeprom/nvm_dynamic_keymap.c:103`) at 32/32, step down through 16, 8, 4 to the highest tier every board builds with, floor 4 tap dance / 2 combo, and report it. `VIAL_KEY_OVERRIDE_ENTRIES 0` (no saved layout uses key overrides). Plus the existing block: `VIALRGB_NO_DIRECT`, `TAPPING_TERM 200`, `PERMISSIVE_HOLD`, `QUICK_TAP_TERM 120`, `HOLD_ON_OTHER_KEY_PRESS_PER_KEY` (ifndef-guarded), commented-out `FLOW_TAP_TERM 150` / `CAPS_WORD_IDLE_TIMEOUT 3000`, `PALETTEFX_ENABLE_ALL_EFFECTS`, `PALETTEFX_ENABLE_ALL_PALETTES`, `RGB_MATRIX_SOLID_REACTIVE_GRADIENT_MODE`.
- Keymap `rules.mk`: `VIA_ENABLE = yes`, `VIAL_ENABLE = yes`, `VIALRGB_ENABLE = yes`, `CAPS_WORD_ENABLE = yes`; `COMMAND_ENABLE = yes` on boards smaller than 80% (RK61, Air75 v2, Q1 Pro is 75% so yes), `no` otherwise; `ENCODER_MAP_ENABLE = yes` on knob boards.
- Keymap `keymap.json`: `{"modules": ["getreuer/socd_cleaner", "getreuer/lumino", "getreuer/palettefx"]}`.
- Keymap `rgb_matrix_user.inc`: `#include "palettefx.inc"`.
- Keymap `keymap.c`: `socd_opposing_pairs` W/S and A/D with `SOCD_CLEANER_LAST`; `get_hold_on_other_key_press` returning `false` for `LT(5, KC_SPC)`, `true` otherwise.
- Built-in keymaps come from the user's saved layouts in `/home/dan/Sync/popos/modern-space-cadet/qmk/vial_2026/*_layer6.vil` (`airv2`, `q3`, `q8`, `rk61`, `v3`, `v4`, `v8`). Each has 6 layers, `LT5(KC_SPACE)` on the spacebar, 13 mouse keys, 4 tap dances (`TD(0..3)`), 1 combo, 0 key overrides. Convert `layout` to `keymaps[]` (and encoder entries to `encoder_map[]` where present); seed the tap dance and combo definitions as Vial defaults so a fresh flash matches the `.vil`. Read-only source: copy, never edit those files. Q1 Pro has no `.vil`: derive it from `q3_layer6.vil` (same Keychron family, knob), mapping keys by position/function onto the 75% layout (author's call).
- `airv2_layer6.vil` uses jincao1 custom keycodes `USER01..05`, `USER08..11`. Map them by function (from jincao1's enum) to ryodeushii's keycodes; escalate any with no equivalent.
- Board/keymap features: `MOUSEKEY_ENABLE = yes` on every board (user uses mouse keys; both new sources default it off).
- Q3/Q8/V3/V4/V8: only the explicit tap dance/combo/key-override counts change; their built-in keymaps may be refreshed from their `.vil` (builder's call, report it).
- Board RGB animations: enable `solid_color`, `cycle_left_right`, and all `solid_reactive*` effects; disable the rest (matches Q3/Air75 v2 today).
- Core: keep the fork's `quantum/qmk_settings.c` change (`get_hold_on_other_key_press` commented out so the keymap's per-key version wins).
- Fork-level: keep the 4 module submodules (`modules/getreuer`, `stasmarkin`, `drashna`, `tzarc`) and `compkeebs.sh` (updated with new board paths and the Q1 Pro).
- Vial UIDs: new boards get a fresh UID from `python3 util/vial_generate_keyboard_uid.py`. Existing boards keep their current UID so saved Vial layouts still match. Unlock combo: two real keys on the board (e.g. Esc + Enter), coordinates read from the board's matrix.

## Job 1: upstream sync

- Fork last merged `vial-kb/vial` on 2026-02-24 (`832bc24554`, merge base `1d9c72972d`). Upstream has 12 newer commits up to `dd43959ae5`, all keyboard additions/fixes; none touch core or the user's boards.
- Merge `vial-kb/vial` into a working branch off `vial`. Expected conflict-free.
- Verify with the current `compkeebs.sh` set before starting Job 2.

## Job 2: RK61 → `royal_kludge/rk61`

- Source: `keyboards/royal_kludge/rk61` from `qmk/qmk_firmware` master (added 2025-10-06, PR #25694). Same MCU, pins, VID/PID (`0x1480/0x6461`), 61-LED layout as `hypharf/rk61`. Not yet in vial-qmk.
- Manufacturer zip (`~/Downloads/qmk_rk61 source code/qmk_rk61`) was audited: stock QMK 0.23.0 + `hypharf/rk61`; its two core edits (bootmagic disabled, unused `rgb_matrix_power_user` hook) are not needed. Nothing from it is ported.
- Copy the upstream board folder; adjust any data-driven keys the vial-qmk schema rejects. Upstream uses the SPI WS2812 driver (`SPID2`, SCK `B13`), which replaces the fork's bit-bang default.
- Add `keymaps/vial` (baseline + layout ported from `hypharf/rk61/keymaps/vial/keymap.c`, keeping the existing UID). Fix the unlock combo (currently `{0,0},{1,1}`).
- Delete `keyboards/hypharf/` once the new board builds. Update `compkeebs.sh`.

## Job 3: Air75 v2 → ryodeushii `nuphy/air75v2`

- Source: `ryodeushii/qmk-firmware` branch `nuphy-keyboards` (active; QMK 0.33.7 base). Bring in `keyboards/nuphy/rules.mk`, `keyboards/nuphy/common/`, `keyboards/nuphy/air75v2/` (ansi only; drop the `ryodeushii` and `srgb` keymaps). Record the source commit hash in the board readme.
- Known shim (proven in a scratch build): `common/core/debounce.c` must use vial-qmk's signatures `void debounce_init(uint8_t num_rows)` and `bool debounce(matrix_row_t raw[], matrix_row_t cooked[], uint8_t num_rows, bool changed)`. With that, `:default` links at 67.6 KB flash.
- Vial keymap: port the layout from the current `nuphy/air75_v2/ansi/keymaps/vial/keymap.c`, renaming jincao1 keycodes to ryodeushii equivalents (`SYS_PRT`, `BAT_NUM`, `KB_SLP`, `LINK_TO` have no direct match today; the builder maps them or escalates). Rebuild `vial.json` from the ryodeushii VIA json and list the custom keycodes in `customKeycodes`.
- ryodeushii defines 45 custom keycodes from `QK_KB_0`, past `QK_KB_31`. Builder must confirm what Vial GUI and vial-qmk accept; if only 32 are usable, keep the wireless/battery/sleep ones within the first 32 and escalate before reordering any enum.
- ryodeushii's custom VIA menus (`common/config/via.c`) will not show in Vial. Keep the defaults; ensure its raw-HID handler cannot swallow Vial's `0xFE` commands.
- Delete `keyboards/nuphy/air75_v2/` (including the `rf_firmware/` zips) once the new board builds. Update `compkeebs.sh`.
- Check RAM (data+bss) against the current build; report it.

## Job 4: Q1 Pro ANSI knob with Bluetooth

- Source: `Keychron/qmk_firmware` branch `wls_2025q1` (QMK 0.28 base). Branch `2025q3` no longer has `q1_pro`. Record the source commit hash.
- Hardware: STM32L432 + CKBT51 Bluetooth module (BT only, no 2.4 GHz).
- Bring in `keyboards/keychron/q1_pro/` (shared files + `ansi_knob/` only) and `keyboards/keychron/bluetooth/` (whole dir; it enables `KC_BLUETOOTH_ENABLE`).
- Keychron's `keyboards/keychron/common/` must not be imported. The Q1 Pro only needs `common/matrix.c` (via `common/common.mk`); place it inside the `q1_pro` folder (or a new Keychron-wireless-only dir) and point the Q1 Pro rules at it. vial-qmk's `common/keychron_common.*` stays untouched.
- Core changes: only those the Q1 Pro needs, each already guarded by Keychron's opt-in flags `RGB_MATRIX_DRIVER_SHUTDOWN_ENABLE` and `RGB_MATRIX_DRIVER_LOAD_ENABLE` (`quantum/rgb_matrix/*`, `drivers/led/snled27351*` for `get_load_ratio`). Skip Keychron's `LK_WIRELESS_ENABLE` / adaptive-NKRO / led_matrix / send_string / report patches unless a link error proves one is needed; escalate in that case. The empty dirs `keyboards/keychron/{q1_pro,bluetooth}` left from an earlier attempt can be removed first.
- Raw HID: Keychron's `via_command_kb` (`0xAA` CKBT51 DFU, `0xAB` factory test) is never called by vial-qmk. Neither handler is ported (see "Calls made by the design author"); the function is deleted. Vial's `0xFE` path must stay untouched.
- Keycodes: `q1_pro.h` already starts at `QK_KB_0` under `VIA_ENABLE`; `BT_HST1..3`, `BAT_LVL` land at `QK_KB_9..12`. `vial.json` `customKeycodes` lists all 13 in enum order.
- `vial.json` from Keychron's `q1_pro` ansi_knob VIA json, with the knob converted to Vial encoder entries.
- Keymap from Keychron's default (Mac/Win base + Fn), extended to 6 layers with the baseline.

## Verification (builder)

- `qmk compile -c -kb <board> -km vial` passes for: `royal_kludge/rk61`, `nuphy/air75v2/ansi`, `keychron/q1_pro/ansi_knob`, `keychron/q3/ansi_encoder`, `keychron/q8/ansi_encoder`, `keychron/v3/ansi_encoder`, `keychron/v4/ansi`, `keychron/v8/ansi_encoder`, plus control `planck/rev6:default`.
- Report `arm-none-eabi-size` per elf, and before/after numbers for the F072 boards.
- Symbols present: `vial_handle_cmd` on all; Q1 Pro also `bluetooth_task`, `ckbt51_*`, `lpm_task`; Air75 v2 the ryodeushii RF/sleep entry points.
- Q3/Q8/V3/V4/V8: confirm `keyboards/keychron/common/` is unchanged and their only diffs are the agreed baseline edits (slot counts, optional keymap refresh); report `.bin` size before/after.
- Each new built-in keymap matches its `.vil` (scripted comparison of keycodes per layer, tap dance, combo).
- Report the chosen tap dance/combo tier with the per-board EEPROM/RAM numbers that justify it.
- The reviewer, not the builder, judges whether this evidence proves the goals.

## Process rules

- Work on a branch off `vial`. No `git add/commit/push/stash` by subagents; commits happen on the main thread, after the user approves each job's result. Suggested one commit (or small series) per job.
- Use `git -C <repo>` on every git call.
- Any decision this document does not settle and that changes behaviour or wire-visible output: stop and escalate.

## Calls made by the design author (not the user; user may override)

- Air75 v2 gets a fresh Vial UID: ryodeushii's custom keycode order differs from jincao1's past `QK_KB_5` (e.g. old `QK_KB_12` `SIDE_SPI` becomes `DEV_RESET`), so restoring an old saved layout onto the new board must not silently remap keys.
- Replaced boards (RK61, Air75 v2) take the new source's defaults (USB PID, debounce, RGB max brightness, etc.) except where the shared baseline says otherwise. The Air75 v2 PID moving from `0x3245` to `0x3246` is accepted.
- Q1 Pro: no Keychron factory test (`ENABLE_FACTORY_TEST` off) and no CKBT51 module-firmware update over raw HID (`0xAA`). Neither handler is ported, so no `quantum/via.c` hook is needed. Updating the BT module firmware requires flashing Keychron stock firmware temporarily.

- (Added during execution) Core gets one weak no-op hook, `dynamic_keymap_reset_user()`, called at the end of `dynamic_keymap_reset()`. Keymaps seed the `.vil` tap dances/combos there and call `vial_init()` to load them, because a reflash of a new build resets EEPROM through `eeconfig_init_via()` without calling `eeconfig_init_user()`.
- (Added during execution) Key overrides are compiled out (`KEY_OVERRIDE_ENABLE = no`); tap dance/combo tier is 16/16 because 32/32 fails the EEPROM assert on the Q3.

## Known hazards (from builder check; handle in board code)

- Q1 Pro `rtc_timer.c` needs LSI: set `STM32_LSI_ENABLED TRUE` in `q1_pro/mcuconf.h` (Keychron set it in core; vial-qmk core has `FALSE`). The build will not catch this.
- Q1 Pro `q1_pro.c` calls `encoder_quadrature_handle_inerrupt_read` (Keychron-patched core). Shim to vial-qmk's `encoder_handle_read()` in board code; do not touch `drivers/encoder`.
- Keychron's `snled27351_get_load_ratio` and driver shutdown functions are unguarded in their tree. Wrap them in `RGB_MATRIX_DRIVER_LOAD_ENABLE` / `RGB_MATRIX_DRIVER_SHUTDOWN_ENABLE` when porting.
- Keychron ships both `q1_pro/matrix.c` and `common/matrix.c` and builds only the latter. Rename or replace so only one matrix implementation compiles.
- Vial regenerates a random BUILD_ID each build, so every flash resets to the built-in keymap until a layout is restored.

## Left to the builder

- Exact keymap layout details beyond the baseline (port from existing keymaps).
- Choice of shim style for API drift, as long as it is in board code.
- Fixing data-driven schema warnings/errors in imported `keyboard.json` / `info.json`.

## Reference

- Recon notes and scratch clones: session scratchpad `src/` (keychron.git, ryo.git, jincao1.git, vial.git).
- Gemini plan corrections: CKBT51 not LKBT51; no 2.4 GHz on Q1 Pro; do not import Keychron `common`; 6 layers not 4.
