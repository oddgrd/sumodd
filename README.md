# Sumo bot

## Development

> IMPORTANT: When the device is powered by battery, and you also want to connect USB for debugging,
> the BATTERY MUST BE CONNECTED FIRST to ensure it is used for powering the device.

To build the firmware:

```sh
# First, generate the build directory. See CMakePresets.json for other presets, including release.
cmake --preset debug
# Compile the firmware.
cmake --build --preset debug
```

To debug the firmware, first build it with the debug preset, then install the probe-rs vscode
extension. You can now start a DAP debug session with probe-rs in the vscode debugging tab.

Flash the firmware with probe-rs:

```sh
probe-rs download --chip STM32F303K8Tx build/debug/sumo.elf --verify
```

Reset the device with probe-rs:

```sh
probe-rs reset --chip STM32F303K8Tx
```

To view logs sent over RTT in debug builds:
```sh
probe-rs attach --chip STM32F303K8TX build/debug/sumo.elf
```

To debug, run the VSCode debugger with the included .vscode/launch.json, but first update the
serial number of the device in configurations[0].probe to your nucleo debugger serial number,
which you can get with:

```sh
probe-rs list
```

If you don't want to use probe-rs, you can convert the .elf file to an ARM binary, then flash with
st-flash:

```sh
# Create the binary
arm-none-eabi-objcopy -O binary build/debug/sumo.elf build/debug/sumo.bin

# Flash it and reset the device
st-flash --reset write  build/debug/sumo.bin 0x08000000
```

## Testing

To run the unity unit tests, run: `./scripts/unit-test.sh`

To run the various integration tests, repeat the build and flash steps above, but use the
desired integration test cmake preset, flashing the resulting binary.

## Hardware

- STM32F303K8T6 MCU with 72MHz CPU (64MHz with HSI), 64 KB flash and 12 KB SRAM. 
    - Data sheet: https://www.st.com/resource/en/datasheet/stm32f303c6.pdf
- Sensors:
    - VL53LOX time-of-flight sensors for detecting enemies.
    - QRE1113 line sensors for detecting the arena edge.
    - TSOP38238 infrared receiver for activating the robot remotely.
- TB6612FNG motor drivers to control the motors.
    - https://cdn.sparkfun.com/assets/0/1/b/b/3/TB6612FNG.pdf
    - https://learn.sparkfun.com/tutorials/tb6612fng-hookup-guide/all
- MPM3610 buck regulator for regulating the voltage to the MCU, ensuring it sees a steady 3.3v,
regardless of battery voltage, which fluctuates with charge.

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
sensors to digital values. We don't need high resolution, we are effectively only seeing high
(black surface, center of dohyo) or low (white surface, border of dohyo). We should consider going
down to 6 or 8 bit in the future, if faster conversions are needed. 

The ADC conversion is not continuous, it is triggered by a timer
peripheral every 20ms (timer clock divided to 1MHz, max count 20000, triggers an event when it maxes out,
which triggers the conversion).

We started out using the STM32 ADC Watchdog, which can be set to trigger an interrupt when one or
more channel conversions are outside of the desired range. However, this interrupt, when covering
many channels, does not tell you which channel triggered it. Therefore, we decided to refactor to
a DMA circular buffer based approach, where the ADC scans all 4 channels every time it is triggered
by the timer event. The resulting conversion values are then placed in a buffer using DMA. When the
scan conversion is completed, an interrupt is raised. In this interrupt we inspect the values of
the buffer to see which channel triggered, which gives us the information we need to decide which
direction to retreat. An event is then emitted to the state machine queue, which handles the rest.

We couldn't use continuous conversion with this strategy, the interrupts would be too frequent, so
we are likely to stick with the timer peripheral conversion trigger. However, we will likely need to
tweak the timings here to be fast enough to work with the robot's speed when we get it up and running.

See ST guide on timer peripheral triggered ADC conversion here:
https://community.st.com/t5/stm32-mcus/using-timers-to-trigger-adc-conversions-periodically/ta-p/49889

### Motor driver

We cannot power the motor directly from the MCU, as it needs 3-6V, and will draw upwards of 60mA
at full speed. The STM32F303K8T6 is rated for at most 25mA from any output pin, and 80mA total
across all pins. Therefore, we will use a MOSFET based H-bridge motor driver, the TB6612FNG.

- It takes power directly from a power source (VM), up to 6V, and uses PWM to control the output
voltage to the motors. Thus the MCU is only indirectly involved in powering the motors.
- The motor driver does not have a clock, the MCU provides the PWM signal using a timer peripheral,
and supplies it to a motor driver input pin. It has two PWM input pins, one for each motor.
- Each motor driver can support up to two motors.
- For each motor, the driver has two additional input pins, used to control the direction of the
motor. These open and close transistors in the H-bridge, which reverses the polarity of the voltage.
- The H-bridge allows it to reverse polarities, thus reversing direction of the brushed DC motors.

#### Motor PWM

It is important that the frequency of the PWM signal is high enough that we avoid current ripples,
which happens when the switching period is slow enough that the motor does not see the average
voltage we want it to see, rather it will see constantly fluctuating voltage, which means the
motor will not spin smoothly.

To generate the PWM, we use a timer peripheral, TIM2 on the MCU. TIM2 is on the APB1 (advanced
peripheral bus 1) bus. The system clock is set to 64MHz, and the APB1 prescaler is set to 1,
so the TIM2 clock is also 64MHz. Furthermore:

- TIM2 is set to count up.
- The TIM2 prescaler (PSC) is set to 1, and the period is set to 1600. The timer will continously
count up to this period value at the clock frequency, then reset. 

We can calculate the PWM frequency from these values.

f_PWM = f_TIM / ((PSC + 1)(ARR + 1))
f_PWM = 64MHz / (2 * 1600)
f_PWM = 20KHz
One period is 50us.

We can control the duty cycle, how long each pulse is HIGH, from our application, by setting the
capture and compare register (CCR) for the timer channel we are using to generate the PWM signal.
When the counter is smaller than the CCR value, the channel output will be HIGH. When it is greater
than or equal to the CCR value, channel output will be low.

For example, if we set the CCR register to 800:

- When the counter is between 0 and 799, the output will be HIGH.
- When the counter is between 799 and 1599, the output will be LOW.

This leaves the PWM duty cycle at 50%, as it is HIGH 50% of the period. The motor driver will use
this signal to switch the voltage it supplies to the motor (from VM) on and off at f_PWM, which
will provide an average voltage to the motor. If the input from VM is 6V, at 800 CCR the motors
will see 3V.

We could adjust the PCK, and set ARR to 100, so that we can easily adjust the duty cycle from 0% to
100% by setting CCR between 0-100, but that gives us poorer resolution. At 100 ARR, a change of 1 is
a change of 1% in voltage. At 1600 ARR, a change of 1 is a 0.0625% change in voltage, which gives us
a lot smoother control of the motor. However, that level of control is likely not important for a
sumo bot, so we may change it later.

### Ranging sensors

For detecting the distance to the enemy robot, we use three ST's VL53L0X sensors, mounted on
Adafruit breakout boards. They connect to the MCU over I2C, in addition to a GPIO pin, for data
ready interrupts, and an XSHUT pin, for reprogramming the I2C address so we can have multiple of
the same sensor connecting on the same bus. The sensors are configured for continuous ranging, and
they will trigger an interrupt via the GPIO pin to an EXTI pin on the MCU when data is ready. That
will toggle a flag in the state machine, which will prompt it to request the latest data over I2C
in the main loop.

With fast mode I2C at 400KHz (which is the max I2C clock frequency for the Vl53L0X), reading from
one sensor once the interrupt is triggered only takes 4ms, whereas with 100KHz it took about 14ms.
If we want to get this down further, we could consider I2C with DMA, but it is likely overkill.

The driver was downloaded from ST: https://www.st.com/en/embedded-software/stsw-img005.html, which
came with platform setup files (platform.c, i2c_platform.c) that were made for Windows, with Windows
dynamic libraries included. We rewrote these to rather work with our STM32 HAL for I2C
communication. The driver API is unaltered, but we wrap it in our own ranging API to make only the
relevant parts available to the robot state machine.

We may write our own driver in the future, since the ST one is quite large, more than doubling the
firmware flash size. However, it is an advanced device with a lot of features, and it's poorly
documented, there is no memory map in the datasheet, for example. This would have to be extracted
from the driver if we were to create a driver.