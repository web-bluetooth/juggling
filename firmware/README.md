# Juggling firmware

Currently runs on the micro:bit, will adapt to nrf51-dk.

1. Install [yotta](http://yottadocs.mbed.com)
2. Run `yotta target bbc-microbit-gcc`
3. Run `yotta install`
4. Run `yotta build`
5. Copy 'build/bbc-microbit-gcc/source/juggling-firmware-combined.hex' to the micro:bit to flash
