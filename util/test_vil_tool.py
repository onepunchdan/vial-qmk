import json
import os
import re
from pathlib import Path
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vil_tool as vt

Q3_VIL = "/home/dan/Sync/popos/modern-space-cadet/qmk/vial_2026/q3_layer6.vil"


class TestVilTool(unittest.TestCase):
    def test_lt_layer_form(self):
        self.assertEqual(vt.convert_keycode("LT5(KC_SPACE)", {}), "LT(5, KC_SPACE)")

    def test_user_to_qk_kb(self):
        self.assertEqual(vt.convert_keycode("USER03", {}), "QK_KB_3")
        self.assertEqual(vt.convert_keycode("USER03", {"USER03": "LINK_BT1"}), "LINK_BT1")

    def test_legacy_names(self):
        for old, new in [("KC_LSHIFT", "KC_LSFT"), ("KC_PSCREEN", "KC_PSCR"),
                         ("KC_SCROLLLOCK", "KC_SCRL"), ("KC_LCTRL", "KC_LCTL"),
                         ("KC_RSHIFT", "KC_RSFT"), ("KC_APPLICATION", "KC_APP")]:
            self.assertEqual(vt.convert_keycode(old, {}), new)

    def test_hex_and_passthrough(self):
        self.assertEqual(vt.convert_keycode("0x7e05", {}), "0x7e05")
        self.assertEqual(vt.convert_keycode("TD(2)", {}), "TD(2)")
        self.assertEqual(vt.convert_keycode("QK_CAPS_WORD_TOGGLE", {}), "QK_CAPS_WORD_TOGGLE")
        self.assertEqual(vt.convert_keycode(-1, {}), "KC_NO")

    def test_sparse_tap_dance_and_combo(self):
        vil = {"layout": [[["KC_A"]]],
               "tap_dance": [["KC_A", "KC_NO", "KC_NO", "KC_NO", 150],
                             ["KC_NO"] * 4 + [200],
                             ["KC_B", "KC_NO", "KC_NO", "KC_NO", 200],
                             ["KC_NO"] * 4 + [200]],
               "combo": [["KC_NO"] * 5]}
        h = vt.vil_to_header(vil, {})
        self.assertIn("VIL_TAP_DANCE_COUNT 3", h)
        self.assertIn("{KC_NO, KC_NO, KC_NO, KC_NO, 200}", h)
        self.assertIn("VIL_COMBO_COUNT 0", h)
        self.assertNotIn("VIL_ENCODER_MAP", h)

    @unittest.skipUnless(os.path.exists(Q3_VIL), "q3 .vil not present")
    def test_header_from_q3_vil(self):
        with open(Q3_VIL) as f:
            vil = json.load(f)
        h = vt.vil_to_header(vil, {})
        self.assertIn("VIL_TAP_DANCE_COUNT 4", h)
        self.assertIn("VIL_COMBO_COUNT 1", h)
        km = h.split("#define VIL_KEYMAPS")[1].split("#define VIL_ENCODER_MAP")[0]
        self.assertEqual(km.count("/* layer "), 6)
        self.assertIn("LT(5, KC_SPACE)", h)
        self.assertIn("ENCODER_CCW_CW(KC_VOLD, KC_VOLU)", h)

    def test_macro_bodies_braced(self):
        vil = {"layout": [[["KC_A"]]], "encoder_layout": [[["KC_A", "KC_B"]]],
               "tap_dance": [], "combo": []}
        h = vt.vil_to_header(vil, {})
        bodies = re.findall(r"#define (VIL_\w+) \\\n(.*?)\n\n", h, re.S)
        self.assertEqual([b[0] for b in bodies],
                         ["VIL_KEYMAPS", "VIL_ENCODER_MAP", "VIL_TAP_DANCE", "VIL_COMBOS", "VIL_QMK_SETTINGS"])
        for name, body in bodies:
            self.assertTrue(body.startswith("{") and body.endswith("}"), name)

    def test_empty_td_slot_term(self):
        vil = {"layout": [[["KC_A"]]],
               "tap_dance": [["KC_NO"] * 4 + [999], ["KC_A", "KC_NO", "KC_NO", "KC_NO", 150]]}
        self.assertIn("{KC_NO, KC_NO, KC_NO, KC_NO, 200}", vt.vil_to_header(vil, {}))

    def test_ensure_keycode_aliases(self):
        for old, new in [("KC_BSLASH", "KC_BACKSLASH"), ("KC_BSPACE", "KC_BACKSPACE"),
                         ("KC_CAPSLOCK", "KC_CAPS_LOCK"), ("KC_LBRACKET", "KC_LEFT_BRACKET"),
                         ("KC_RBRACKET", "KC_RIGHT_BRACKET"), ("KC_SCOLON", "KC_SEMICOLON"),
                         ("KC_NUMLOCK", "KC_NUM_LOCK"), ("KC_PGDOWN", "KC_PAGE_DOWN"),
                         ("KC_LSPO", "QK_SPACE_CADET_LEFT_SHIFT_PARENTHESIS_OPEN"),
                         ("KC_RSPC", "QK_SPACE_CADET_RIGHT_SHIFT_PARENTHESIS_CLOSE"),
                         ("MAGIC_UNHOST_NKRO", "QK_MAGIC_NKRO_OFF")]:
            self.assertEqual(vt.convert_keycode(old, {}), new)
        self.assertEqual(vt.convert_keycode("KC_LSHIFT", {}), "KC_LSFT")

    @unittest.skipUnless(os.path.exists(Q3_VIL), "q3 .vil not present")
    def test_q3_identifiers_defined(self):
        with open(Q3_VIL) as f:
            h = vt.vil_to_header(json.load(f), {})
        h = re.sub(r"/\*.*?\*/|//.*", "", h)
        idents = {i for i in re.findall(r"[A-Za-z_]\w*", h) if not i.startswith("VIL_")}
        idents -= {"define", "pragma", "once", "TD", "LT", "MO", "TO", "TG", "ENCODER_CCW_CW"}
        text = ""
        root = Path(vt.REPO)
        for pat in ("quantum/**/*.h", "tmk_core/**/*.h"):
            for f in root.glob(pat):
                if f.name != "vial_ensure_keycode.h":
                    text += f.read_text(errors="ignore")
        missing = sorted(i for i in idents
                         if not re.search(r"\b%s\b" % re.escape(i), text))
        self.assertEqual(missing, [])

    def test_qmk_settings_macro(self):
        vil = {"layout": [[["KC_A"]]], "settings": {"27": 150, "2": 50, "7": 200}}
        h = vt.vil_to_header(vil, {})
        self.assertIn("#define VIL_QMK_SETTINGS \\\n{ \\\n    {2, 50}, \\\n    {7, 200}, \\\n    {27, 150}, \\\n}\n", h)
        self.assertIn("#define VIL_QMK_SETTINGS_COUNT 3", h)

    def test_qmk_settings_absent(self):
        h = vt.vil_to_header({"layout": [[["KC_A"]]]}, {})
        self.assertIn("#define VIL_QMK_SETTINGS_COUNT 0", h)

    def test_q3_header_has_settings(self):
        with open(Q3_VIL) as f:
            h = vt.vil_to_header(json.load(f), {})
        m = re.search(r"VIL_QMK_SETTINGS_COUNT (\d+)", h)
        self.assertTrue(m and int(m.group(1)) > 0)

    def test_vialjson_skips_odd_legend(self):
        kle = {"layouts": {"keymap": [["abc", "0,0"]]}}
        self.assertEqual(vt.vialjson_positions(kle), {(0, 0)})

    def test_vialjson_positions(self):
        kle = {"layouts": {"keymap": [["0,0", "0,1"], ["0,0\n\n\n\n\n\n\n\n\ne"]]}}
        self.assertEqual(vt.vialjson_positions(kle), {(0, 0), (0, 1)})

    def test_vialjson_positions_skips_props_and_options(self):
        kle = {"layouts": {"keymap": [[{"x": 1}, "0,0\n\n\n1,0", "0,1"]]}}
        self.assertEqual(vt.vialjson_positions(kle), {(0, 0), (0, 1)})


if __name__ == "__main__":
    unittest.main()
