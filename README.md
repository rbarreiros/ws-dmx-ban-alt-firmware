# WORK IN PROGRESS! NOT YET WORKING!!!


# ws-dmx-ban-alt-firmware
This repository contains an alternative firmware for the WS-DMX-xch ban dmx led drivers.
Those drivers are available from several resellers in different configurations.
So far this code has only been tested on the WS-DMX-8CH BAN rev 4.1.

The WS-DMX driver is attractive because it is cheap but its stock firmware is fairly basic. Luckily they contain an Intel 8051 compatible chip and expose the ISP pins.

This project aims to extend the firmware to include a master intensity channel and a strobe channel.
Further improvements like different dimming curves, smoother dimming etc. might also be added.



![Alt text](programmer_pintout.jpg?raw=true "WS-DMX-8ch BAN pinout")

# dependencies
```bash
sudo apt install sdcc
pip3 install stcgal
```
# compiling
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=../sdcc.toolchain ..
```

# flashing

- Connect RX/TX/GND from FT232 adapter according to the pinout image.
- Set the FT232 to 3.3V
- Connect external 5v to the board
- turn off board
- start flashing
- turn on board

```bash
stcgal -P stc15 -p /dev/ttyUSB0  dmx.ihx
```




