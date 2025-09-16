#!/usr/bin/bash

qmk compile -c -kb hypharf/rk61 -km vial && qmk compile -kb keychron/v4/ansi -km vial && qmk compile -kb keychron/v3/ansi_encoder -km vial && qmk compile -kb keychron/v8/ansi_encoder -km vial && qmk compile -kb keychron/q3/ansi_encoder -km vial && qmk compile -kb keychron/q8/ansi_encoder -km vial
