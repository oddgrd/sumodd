# Sumodd mini-sumo robot

The idea for this project, the design of the firmware statemachine, as well as a lot of the
hardware choices were heavily inspired by [`artfulbytes`](https://github.com/artfulbytes)
excellent youtube series where he made a sumo robot from scratch. See his project code
[here](https://github.com/artfulbytes/nsumo_video/tree/main).

## Hardware overview

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
- [Motor control](docs/motor-control.md).

## Development

This project uses [`mise`](https://mise.jdx.dev/) to manage tools and commands/tasks. If you'd
rather not use it, inspect the `mise.toml` file to read which tools are required, and which
commands it runs.

For more information on command usage, append --help to a command, e.g. `mise run build --help`.
For an interactive overview of the available commands, run `mise run`.

### Building

```sh
# Defaults to debug
mise run build

# Alternatively, specify build preset
mise run build release
```

### Flashing

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

### Debugging

To debug the firmware, first build it with the debug preset, then install the probe-rs vscode
[extension](https://marketplace.visualstudio.com/items?itemName=probe-rs.probe-rs-debugger).
You can now start a DAP debug session with probe-rs in the vscode debugging tab.

- To launch a new debug session, which flashes first and resets the device, run the VSCode debugger
with the `.vscode/launch.json` "probe-rs launch" configuration.
- Alternatively, to attach a debugger to a running device to inspect the current state, run the VS
Code debugger with the `.vscode/launch.json` "probe-rs attach" configuration

You can also view logs sent over RTT in debug or integration test builds:

```sh
# Build preset is required, defaults to debug
mise run log
```

### Flashing without probe-rs

The mise commands depend on probe-rs. If you don't want to use probe-rs for flashing and resetting,
you can convert the .elf file to an ARM binary, then flash with st-flash, or whichever other method
you prefer:

```sh
# Create the binary
arm-none-eabi-objcopy -O binary build/debug/sumo.elf build/debug/sumo.bin

# Flash it and reset the device
st-flash --reset write  build/debug/sumo.bin 0x08000000
```

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

## Diagrams

[PlantUML](https://plantuml.com/) is used for diagrams, it depends on Java, which mise manages.
Furthermore, you need to install [graphviz](https://plantuml.com/graphviz-dot) on your system,
as mise cannot manage it.

To download the jar file for platuml: `mise run plantuml-setup`. The path it downloads to is set
in a mise variable, which may need to be adjusted to your local system.

To generate a UML png: `mise run plantuml-generate`. Defaults to state machine diagram.
