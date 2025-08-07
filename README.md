# USB PD PSU
Simple USB Power Delivery PSU with voltage, current and power measurements.

![board](docs/img/board.jpg)

Designe around ProMicro (NiceNano!) nRF52840 board features:
- the nRF52840 MCU
- 3x INA219 
- SSD1306 OLED screen
- 4x buttons (3x user and reset)

The PSU negotiates with a USB-C charger for a voltage up to 20V. 
Two voltages can be set with the onboard potetiometers.
The third supplies the MCU, OLED display and cannot be changed!

All communication happens via I2C. Measurement chips are configured for addresses: `0x40, 0x41 and 0x44`. The display is under address `0x3C`.

## Compile and run
Inside a zephyr environment:
```
west build -b promicro_nrf52840/nrf52840/uf2
west flash
```

## Copyright
2025 G2Labs Grzegorz Grzęda under MIT license