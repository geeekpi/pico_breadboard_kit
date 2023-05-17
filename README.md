# pico_breakboard_kit
## Description
This is Full Demo Code for Pico Breakboard Kit with 3.5" TFT Capacitive Touch Screen
## TFT Features
* Resolution: 320x480 Pixels
* Screen IC: ST7796SU1
* Input Voltage: 3.3V 
* Touch Type: Capacitive Touch Screen

## Getting Start
* Install Pico-SDK in Raspberry Pi 
Assume that you are using Raspberry Pi OS 64bit (Bullseye) as operating system on Raspberry Pi. Here we are using Raspberry Pi 4B as PC to compile the project.

> NOTE: Please make sure your Raspberry Pi can access internet and GitHub website. 
Login to Desktop and open a terminal, typing following command to build Raspberry Pi Pico-SDK environment.
These instructions are extremely terse, and Linux-based only. 
For detailed steps, instructions for other platforms, and just in general, we recommend you see [Raspberry Pi Pico C/C++ SDK](https://rptl.io/pico-c-sdk)
1. Install CMake (at least version 3.13), and GCC cross compiler

```bash 
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```
2. Set up your project to point to use the Raspberry Pi Pico SDK

  * Either by cloning the SDK locally (most common) :

    a. git clone this Raspberry Pi Pico SDK repository

    b. Copy [pico_sdk_import.cmake](https://github.com/raspberrypi/pico-sdk/blob/master/external/pico_sdk_import.cmake) from the SDK into your project directory

    c. Set `PICO_SDK_PATH` to the SDK location in your environment, or pass it (`-DPICO_SDK_PATH=`) to cmake later.

    d. Setup a CMakeLists.txt like:
    ```bash
    cmake_minimum_required(VERSION 3.13)

    # initialize the SDK based on PICO_SDK_PATH
    # note: this must happen before project()
    include(pico_sdk_import.cmake)

    project(my_project)

    # initialize the Raspberry Pi Pico SDK
    pico_sdk_init()

    # rest of your project
    ```
* Download Repository 
```bash
cd /home/pi/
git clone --recursive https://github.com/geeekpi/pico_breakboard_kit.git
```

* Build Projects
```bash
cd pico_breakboard_kit/
mkdir build
cd build/
cmake ..
make -j4
```

* Upload firmware to Pico 
Unplug Raspberry Pi Pico from Raspberry Pi and press `boot_sel` button and then connect the Raspberry Pi Pico back to Raspberry Pi.
Execute following command to copy the `*.uf2` file to Pico. 
```bash
cp firmware.uf2 /media/pi/RPI-RP2/
```
After a while, when the firmware has been uploaded to Pico, it will restart automatically, you can test the demo code according to the information on screen. 
Have fun!
## FAQ
* Why is is so slow when I drag the circle ring on screen? 
Because of the memory of pico is just 264KB, and graphic interface may consume a lot of memory to show the graphic widget. 
* Can I use MicroPython with LVGL to drive the screen?
No, it is lack of memory, so it may stack when you upload the firmware. 
