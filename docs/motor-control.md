# Motor control

## Hardware

- [DRV8212][DRV8212] motor drivers, one for each motor. For more information on the motor control
hardware, see the [motor control section](https://github.com/oddgrd/sumodd-hardware#motor-control)
of the sumodd-hardware repository.

The motors cannot be powered directly from the MCU, as they will need higher voltages than the MCU
can supply, and significantly higher currents. The STM32F303K8T6 is rated for at most 25mA from
any output pin, and 80mA total across all pins, whereas our motors at the time of writing have a
an idle current of 120mA, and stall current of 3.2A. Furthermore, we are using brushed DC motors,
so we also need to be able to reverse the supply polarity, to reverse the direction the motors
spin. Therefore, we will use a MOSFET based H-bridge motor driver, which can control two motors.

In earlier iterations a dual-channel TB6612FNG driver was used, but to support higher currents,
the current version uses two [DRV8212][DRV8212] motor drivers.

- It takes motor power directly from our battery on the VM pin, and it can output up to 4A
continuous current per driver.
- It uses a PWM signal from the MCU to control the output voltage to the motors, which allows us
to control the speed of the motors by adjusting the duty cycle.
- It uses a GPIO input to control polarity, if it is LOW the polarity is OUT2 -> OUT1, and if it
is high it is the opposite, which results in clockwise or counterclockwise rotation, depending on
how the motors are connected.

### Motor PWM

It is important that the frequency of the PWM signal is high enough that we reduce current ripples,
which happens when the switching period is slow enough that the motor does not see the average
voltage we want it to see, rather it will see signficantly fluctuating voltage, which means the
motor will not spin smoothly.

To generate the PWM signal, we use a timer peripheral on the MCU. The PWM frequency (f_PWM) is
determined by the timer input clock (f_TIM), the prescaler register (PSC), and the auto-reload
register (ARR):

`f_PWM = f_TIM / ((PSC + 1) * (ARR + 1))`

For example, if the timer input clock is 64 MHz and we configure:

```
PSC = 31
ARR = 99
```

then:

```
f_PWM = 64 MHz / ((31 + 1) * (99 + 1))
      = 64 MHz / (32 * 100)
      = 20 kHz
```

This gives a PWM period of 50 µs.

We can control the duty cycle, how long each pulse is HIGH, from our application, by setting the
capture and compare register (CCR) for the timer channel we are using to generate the PWM signal.
When the counter is smaller than the CCR value, the channel output will be HIGH. When it is greater
than or equal to the CCR value, channel output will be low.

For example, if we set the CCR register to 50:

- When the counter is between 0 and 50, the output will be HIGH.
- When the counter is between 50 and 100, the output will be LOW.

This leaves the PWM duty cycle at 50%, as it is HIGH 50% of the period. The motor driver will use
this signal to switch the voltage it supplies to the motor (from VM) on and off at f_PWM, which
will provide an average voltage to the motor. If the input from VM is 6V, at 50 CCR the motors
will see 3V.

If we attach an oscilloscope to the PWM outputs from the MCU, we can verify it has the expected
20kHz frequency, as well as the duty cycle we set with the CCR register.

#### Oscilloscope captures of motor control PWM output pins

First, lets look at both channels, symmetrically set to 25% duty cycle, meaning both motors are
running at the same speed:

![Oscilloscope motor PWM 25% duty cycle symmetric two channels](media/pwm-symmetric-25.png)

And 75% duty cycle:

![Oscilloscope motor PWM 75% duty cycle symmetric two channels](media/pwm-symmetric-75.png)

But we can also control the motors asymetrically, for example if we want to do a wide arc turn,
we can set one motor to a higher speed than the other, so the motor turns in the direction of the
slow motor. Here we set the left motor to 25% duty cycle, and the right to 50%, so the robot will
turn gradually towards the left.

![Oscilloscope motor PWM 25%/50% duty cycle asymmetric two channels](media/pwm-asymmetric-25-50.png)

[DRV8212]: https://www.ti.com/lit/ds/symlink/drv8212.pdf