#pragma once

#define NUPHY_VIA_EEPROM_CUSTOM_CONFIG_SIZE 23

// vial-qmk has no VIA custom-config EEPROM region; Vial builds use the eeconfig kb datablock.
#if !defined(VIA_ENABLE) || defined(VIAL_ENABLE)
#    ifndef EECONFIG_KB_DATA_SIZE
#        define EECONFIG_KB_DATA_SIZE NUPHY_VIA_EEPROM_CUSTOM_CONFIG_SIZE
#    endif
#endif

#if defined(VIA_ENABLE) && !defined(VIAL_ENABLE)
#    define VIA_EEPROM_CUSTOM_CONFIG_SIZE NUPHY_VIA_EEPROM_CUSTOM_CONFIG_SIZE
#endif
