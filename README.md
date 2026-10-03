# hid-controller

A custom USB gamepad built on an STM32F103 (KiCad PCB + firmware). It has two analog sticks, a D-pad, face buttons, bumpers, triggers and Start/Share, and shows up on the host as an Xbox 360-style wired controller.

<img src="assets/physical_device.jpg" alt="Assembled controller" width="420">

▶ [Watch the demo video](assets/controller_video.mp4)

Linux enumerates it fully. Windows only partly enumerates it so far.

## Build the PCB

The KiCad project is in `hid-controller-pcb/`. Gerbers are in `hid-controller-pcb/production/hid-controller-pcb.zip`, and the BOM is `hid-controller-pcb/BOM.csv`. Upload the zip to any PCB fab.

<img src="assets/pcb_layout.png" alt="PCB layout" width="420">

## Build and flash the firmware

Requires `arm-none-eabi-gcc` (the Makefile expects it in `/usr/bin/`; override with `make GCC_PATH=<dir>`), an SWD programmer such as an ST-Link, and [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) for `STM32_Programmer_CLI`.

```sh
cd hid-controller-code/hid-controller-code
make
STM32_Programmer_CLI -c port=SWD -w build/hid-controller-code.elf -v -rst
```
