# Sumo bot

### Hardware

- STM32F303K8T6 MCU with 72MHz CPU, 64 KB flash and 12 KB SRAM. 
- Sensors:
    - VL53LOX time-of-flight sensors for detecting enemies.
    - QRE1113 line sensors for detecting the arena edge.
    - TSOP38238 infrared receiver for activating the robot remotely.
- TB6612FNG motor drivers to control the motors.
- MPM3610 buck regulator for regulating the voltage to the MCU.

### Development

To build the firmware:

```sh
# See CMakePresets.json for other presets, including Release
cmake --build --preset Debug
```

To debug the firmware, first build it with the Debug preset, then install the probe-rs vscode
extension. You can now start a DAP debug session with probe-rs in the vscode debugging tab.

Flash the firmware with probe-rs:

```sh
probe-rs download --chip STM32F303K8Tx build/Debug/sumo.elf --verify
```

Reset the device with probe-rs:

```sh
probe-rs reset --chip STM32F303K8Tx
```

Alternatively, you can convert the .elf file to an ARM binary, then flash with st-flash:

```sh
# Create the binary
arm-none-eabi-objcopy -O binary build/Debug/sumo.elf build/Debug/sumo.bin

# Flash it and reset the device
st-flash --reset write  build/Debug/sumo.bin 0x08000000
```

### NEC

Uses TIM16 as a 1Mhz input capture channel, used to time the pulses of the IR receiver, triggered
on falling edges only, for simplicity.

With an NEC protocol[^1] receiver, we will receive 32 bits for each keypress.

- 9ms down and 4.5ms up represents start of transmission.
- Next, we will receive 32 pulses representing bits, ~1.1ms for b0 and ~2.2ms for b1.
- The first 16 bits are the address of the remote device. Note that our remote is an NECx remote,
in normal NEC the first 16 bits are address and address inverted.
- Followed by 8 bit command, and inverted 8 bit command.

[^1]: https://www.infineon.com/assets/row/public/documents/60/42/infineon-an2023-03-infrared-remote-control-and-saving-last-speed-setting-applicationnotes-en.pdf?fileId=8ac78c8c8d1b852e018d21ff0aa71feb
