# RF Firmware

These RF firmware is shared to NuPhy users via e-mails and subsequently published to their official Discord by those users.
It is the firmware that controls the `NRF52810` chipset that does the Bluetooth and 2.4GHz dongle transmissions. It also handles synchronization of data such as battery level.

My firmware is tested on `v1.0.3`, which I personally use, but it should be fully compatible on `v1.0.7`. Consider updating the RF firmware if you have wireless issues.

# Flashing

Refer to the [Flashing Instructions](Instructions_on_Flashing_the_RF_Firmwares.docx) for updating the RF module. You can also put the module into `DFU` mode by mapping the `RF DFU` key in VIA.

# Compatibility

The RF firmware is compatible on Air75 V2, Air60 V2 and Air96 V2 since all 3 boards use the same hardware.

# Improvements

Users have reported potential improvements on wireless range and connectivity by updating to the latest firmware. Consider updating if you face issues with wireless connectivity but please see the disclaimer section as well.

# Disclaimer

These firmware are closed source provided by NuPhy. I do not have changelogs and do not know what the difference between the versions are. Supposedly, per NuPhy staff, each iteration is supposed to improve upon the RF performance and stability but no empirical evidence from my end supports that (since I don't have major issues overall).

As I do not have the version that shipped with the board you **cannot** go back to the original firmware if anything goes wrong (but it shouldn't). You can, however, flash to another RF firmware version.

Consider this a case of `if it ain't broke, don't fix it` scenario if the stock firmware works for you.
