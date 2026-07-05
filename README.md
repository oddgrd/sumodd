# Sumodd mini-sumo robot

The idea for this project, the design of the firmware statemachine, as well as a lot of the
hardware choices were heavily inspired by [`artfulbytes`](https://github.com/artfulbytes)
excellent youtube series where he made a sumo robot from scratch. See his project code
[here](https://github.com/artfulbytes/nsumo_video/tree/main).

## Development

This project uses [`mise`](https://mise.jdx.dev/) to manage tools and commands/tasks. If you'd
rather not use it, inspect the `mise.toml` file to read which tools are required, and which
commands it runs.

For more information on command usage, append --help to a command, e.g. `mise run build --help`.
For an interactive overview of the available commands, run `mise run`.

To build the firmware:

```sh
# Defaults to debug
mise run build

# Alternatively, specify build preset
mise run build release
```

To debug the firmware, first build it with the debug preset, then install the probe-rs vscode
extension. You can now start a DAP debug session with probe-rs in the vscode debugging tab.

To build, flash the firmware and reset the device:
```sh
# Build preset is required, defaults to debug
mise run flash
```

To build, flash, reset and attach an RTT logger to device:

```sh
# Build preset is required, defaults to debug
mise run launch
```

To reset the device:

```sh
mise run reset
```

To view logs sent over RTT in debug or integration test builds:

```sh
# Build preset is required, defaults to debug
mise run log
```

To debug, run the VSCode debugger with the included .vscode/launch.json, but first update the
serial number of the device in configurations[0].probe to your nucleo debugger serial number,
which you can get with:

```sh
probe-rs list
```

The mise commands depend on probe-rs. If you don't want to use probe-rs for flashing and resetting,
you can convert the .elf file to an ARM binary, then flash with st-flash, or whichever other method
you prefer:

```sh
# Create the binary
arm-none-eabi-objcopy -O binary build/debug/sumo.elf build/debug/sumo.bin

# Flash it and reset the device
st-flash --reset write  build/debug/sumo.bin 0x08000000
```

## Diagrams

[PlantUML](https://plantuml.com/) is used for diagrams, it depends on Java, which mise manages.
Furthermore, you need to install [graphviz](https://plantuml.com/graphviz-dot) on your system,
as mise cannot manage it.

To download the jar file for platuml: `mise run plantuml-setup`. The path it downloads to is set
in a mise variable, which may need to be adjusted to your local system.

To generate a UML png: `mise run plantuml-generate`. Defaults to state machine diagram.

## Testing

To run the unity unit tests, run: `mise run test`

To run the various integration tests, connect the device, then repeat the build and flash steps
above, but use the desired integration test cmake preset.

The currently existing integration test presets are:

- test-drive

For example, if you want to build, flash and attach a logger to an integration test, run:

```sh
mise run launch test-drive
```

## Hardware

- STM32F303K8T6 MCU with 72MHz CPU (64MHz with HSI), 64 KB flash and 12 KB SRAM. 
    - Data sheet: https://www.st.com/resource/en/datasheet/stm32f303c6.pdf
- Sensors:
    - VL53LOX time-of-flight sensors for detecting enemies.
    - QRE1113 line sensors for detecting the arena edge.
    - TSOP38238 infrared receiver for activating the robot remotely.
- TB6612FNG motor driver to control the motors.
- MPM3610 buck regulator for regulating the voltage to the MCU, ensuring it sees a steady 3.3v,
regardless of battery voltage, which fluctuates with charge.
- 6v, 500RPM geared brushed DC motors.
    - https://www.jsumo.com/mp12-micro-gear-motor-6v-500rpm
- 33mm diameter, aluminium wheels with high-friction rubber.
    - https://www.jsumo.com/slt20-aluminum-silicone-wheel-set-33mmx20mm-pair

## Documentation

Documentation of the robot's core functionality can be found in the docs directory.

- [Dohyo border line detection](docs/line-detection.md).
- [Remote start IR signal handling](docs/ir-remote.md).
- [Enemy detection with IR ToF sensors](docs/ranging.md).

TODO: extract, and expand, documentation of remaining core functionality to docs directory.

### Motor driver

We cannot power the motors directly from the MCU, as they need 3-6V, and will draw up to 0.8A each
when stalling. The STM32F303K8T6 is rated for at most 25mA from any output pin, and 80mA total
across all pins. Therefore, we will use a MOSFET based H-bridge motor driver, the TB6612FNG.

- It takes power directly from a power source (VM), up to 6V, and uses PWM to control the output
voltage to the motors. Thus the MCU is only indirectly involved in powering the motors.
- The motor driver does not have a clock, the MCU provides the PWM signal using a timer peripheral,
and supplies it to a motor driver input pin. It has two PWM input pins, one for each motor.
- For each motor, the driver has two additional input pins, used to control the direction of the
motor. These open and close transistors in the H-bridge, which reverses the polarity of the voltage.
This can be used to control the direction of the motors, clockwise or counter-clockwise.

#### Motor PWM

It is important that the frequency of the PWM signal is high enough that we avoid current ripples,
which happens when the switching period is slow enough that the motor does not see the average
voltage we want it to see, rather it will see constantly fluctuating voltage, which means the
motor will not spin smoothly.

To generate the PWM, we use a timer peripheral, TIM2 on the MCU. TIM2 is on the APB1 (advanced
peripheral bus 1) bus. The system clock is set to 64MHz, and the APB1 prescaler is set to 32,
so the TIM2 clock is 2 MHz. Furthermore:

- TIM2 is set to count up.
- The TIM2 prescaler (PSC) is set to 32, and the period is set to 100. The timer will continously
count up to this period value at the clock frequency, then reset. 

We can calculate the PWM frequency from these values.

f_PWM = f_TIM / ((PSC + 1)(ARR + 1))
f_PWM = 64MHz / (32 * 100)
f_PWM = 20KHz
One period is 50us.

We can control the duty cycle, how long each pulse is HIGH, from our application, by setting the
capture and compare register (CCR) for the timer channel we are using to generate the PWM signal.
When the counter is smaller than the CCR value, the channel output will be HIGH. When it is greater
than or equal to the CCR value, channel output will be low.

For example, if we set the CCR register to 50:

- When the counter is between 0 and 49, the output will be HIGH.
- When the counter is between 49 and 99, the output will be LOW.

This leaves the PWM duty cycle at 50%, as it is HIGH 50% of the period. The motor driver will use
this signal to switch the voltage it supplies to the motor (from VM) on and off at f_PWM, which
will provide an average voltage to the motor. If the input from VM is 6V, at 49 CCR the motors
will see 3V.
