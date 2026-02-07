# Sumo bot

### Hardware

- STM32F303K8T6 MCU with 72MHz CPU, 64 KB flash and 12 KB SRAM. 
    - Data sheet: https://www.st.com/resource/en/datasheet/stm32f303c6.pdf
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

### IR Remote receiver

Uses TIM16 as a 1Mhz input capture channel, used to time the pulses of the IR receiver. It triggers
on each falling edge, for simplicity, and that captures the current count of the timer with exact
precision, regardless of is MCU is busy. We keep the count in memory, and on the next falling edge,
we compare the two to gauge the duration of the pulse. Each tick of the counter represents 1us at
1MHz clock. We can use this information to decode the signal.

See ST guide on input capture here:
https://community.st.com/t5/stm32-mcus/how-to-use-the-input-capture-feature/ta-p/704161

With an NEC protocol[^1] receiver, we will receive 32 bits for each keypress.

- 9ms down and 4.5ms up represents start of transmission.
- Next, we will receive 32 pulses representing our bits, ~1.1ms for b0 and ~2.2ms for b1.
- The first 16 bits are the address of the remote device. Note that our remote is an NECx remote,
in conventional NEC the first 16 bits are address and address inverted.
- The address bits are followed by 8 bit command, and inverted 8 bit command, which can be used
to validate the integrity of the signal, if it is not correctly inverted, we should discard the
message.

[^1]: https://www.infineon.com/assets/row/public/documents/60/42/infineon-an2023-03-infrared-remote-control-and-saving-last-speed-setting-applicationnotes-en.pdf?fileId=8ac78c8c8d1b852e018d21ff0aa71feb

### Line-sensor ADC

We use one of the on-chip ADCs at 10bit resolution to convert the analog readings from the QRE1113 line
sensors to digital values. The ADC conversion is not continuous, it is triggered by a timer
peripheral every 20ms (timer clock divided to 1MHz, max count 20000, triggers an event when it maxes out).
Then, an analog watchdog is configured to trigger an interrupt when a conversion value is outside of the
desired range, which has been arrived at by manual testing to be whenever a line is encountered.

We could use continuous conversion, but to reduce power usage, and to reduce the frequency of
interrupts triggered by the watchdog, we use the timer output trigger strategy. We will need to
tweak the timings here to be fast enough to work with the robot's speed when we get it up and running.

See ST guide on this here:
https://community.st.com/t5/stm32-mcus/using-timers-to-trigger-adc-conversions-periodically/ta-p/49889

Currently, it is only configured to work with one line-sensor, but it should be possible to have
an ADC watchdog watch all 4 line sensors, and hopefully also to distinguish them, so we can know
which sensor, and thus corner of the bot, has crossed the line.

