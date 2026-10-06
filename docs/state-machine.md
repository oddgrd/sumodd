# State machine

Our firmware is centered around a finite state machine, defined in `app/state.c`. In the firmware
main function, we run a while loop that checks sensor inputs on each iteration, end emits an event
depending on the input. This event is fed into the state machine, and in combination with the
current state, it arrives at the next state using a lookup table of valid state transitions.

![Sumo robot state machine diagram](media/state.png)

Note that we do not simply poll the sensor inputs on each iteration, we rely on interrupts and DMA
to avoid blocking the main loop. For more information on that, see the documentation for [line
sensors](line-detection.md) and [ranging sensors](ranging.md).