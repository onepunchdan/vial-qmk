#!/usr/bin/bash

qmk compile -c -kb hypharf/rk61 -km vial && qmk compile -c -kb keychron/v4/ansi -km vial && qmk compile -c -kb keychron/v3/ansi_encoder -km vial && qmk compile -c -kb keychron/v8/ansi_encoder -km vial && qmk compile -c -kb keychron/q3/ansi_encoder -km vial && qmk compile -c -kb keychron/q8/ansi_encoder -km vial && qmk compile -c -kb nuphy/air75_v2/ansi -km vial
