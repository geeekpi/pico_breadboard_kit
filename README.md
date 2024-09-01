# pico_breakboard_kit
## Description
This is Full Demo Code for Pico Breakboard Kit with 3.5" TFT Capacitive Touch Screen
## TFT Features
* Resolution: 320x480 Pixels
* Screen IC: ST7796SU1
* Input Voltage: 3.3V 
* Touch Type: Capacitive Touch Screen
* TFT screen communication protocol: SPI (SPI0)
* TFT Capacitive touch screen communication protocol: I2C (I2C0 SDA: GP8, SCL: GP9)
## Pinout 
| Components | Pinout|
|---|---|
| Buzzer | GP13 |
| LEDs | D1: GP16, D2: GP17, D3: 3V3, D4: 5V |
| RGB LED| GP12|
| Joystick| X-axis: ADC0, Y-axis: ADC1 
|Button | BTN1: GP15, BTN2: GP14|

### TFT screen Pinout
|Raspberry Pi Pico | 3.5 TFT Screen |
|---|---|
| GP2 | CLK |
| GP3 | DIN |
| GP5 | CS |
| GP6 | DC |
| GP7 | RST |
### Touch Screen Pinout
|Raspberry Pi Pico | Capacitive Touch screen |
|---|---|
| I2C0 SDA GP8 | SDA |
| I2C0 SCL GP9 | SCL |

## Pico 2 Dev Branch Note

```bash 
export FREERTOS_KERNEL_PATH=~/pico_breakboard_kit/components/FreeRTOS
# ARM (You can only choose one.)
cmake -DCMAKE_BUILD_TYPE=Debug -DPICO_PLATFORM=rp2350 -DPICO_BOARD=pico2 -DPICO_RP2350=1 FREERTOS_KERNEL_PATH=~/pico_breakboard_kit/components/FreeRTOS ..
# RISC-V (need build toolchain by yourself!)
# cmake -DCMAKE_BUILD_TYPE=Debug -DPICO_PLATFORM=rp2350-riscv -DPICO_BOARD=pico2 -DPICO_RP2350=1 FREERTOS_KERNEL_PATH=~/pico_breakboard_kit/components/FreeRTOS ..
make
```

* Upload firmware to Pico 2
Unplug Raspberry Pi Pico 2 from Raspberry Pi and press `boot_sel` button and then connect the Raspberry Pi Pico back to Raspberry Pi.
Execute following command to copy the `*.uf2` file to Pico. 
```bash
cp firmware.uf2 /media/pi/RP2350/
```
After a while, when the firmware has been uploaded to Pico, it will restart automatically, you can test the demo code according to the information on screen. 
Have fun!

## Special Announcement
This is a test branch, not guaranteed in the sale, and will run slower than the RP2040 because it is currently not possible to use multi-core optimisation, pending a FreeRTOS kernel update.
