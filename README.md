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

TODO
- Create a struct to represent the 32 bit signal. Union of 32 bit integer or struct of 4 invidual 8
bit values.
- When we receive a pulse, stop the timer, increment a variable that holds the pulse count. We will
receive 3 pulses before we start receiving the bits, and the last pulse will be 34. Any pulse
inbetween will represent the bits of a message. After we have decoded the pulse and added the result
to the message ring buffer, restart the timer.
- b1 will 2.25ms, b0 will be 1.5ms. We will count any time greater than 2ms as b1, and the shorter
as b0.
- Add message to ring buffer.
- If we keep the button pressed, it will keep sending the same message at a repeating interval.

Youtube:
https://www.youtube.com/watch?v=rh4pdNWKLJY
https://www.youtube.com/watch?v=K7eHkij-wNY