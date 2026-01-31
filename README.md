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

TIM16 1Mhz input capture channel, used to time the pulses of the IR receiver, triggered on falling
edges.

With an NEC protocol receiver, we will receive 32 bits for each keypress.

- 9ms down and 4.5ms up represents start of transmission.
- Followed by 8 bit address, and then inverse of 8 bit address. This is going to be the same for our
IR remote, so we will ignore this.
- Followed by 8 bit command, and inverted 8 bit command.

We only track the time between the falling edges, for simplicity. 
