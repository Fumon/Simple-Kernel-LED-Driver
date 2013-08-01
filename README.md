#Simple Kernel Module

This is a simple loadable kernel module which I wrote to get familiar with some of the subsystems involved for an interview.

The module has a virtual LED variable `led_on` which it "flashes" while a virtual button, `bbb`, is being "pressed" and stops when it is "unpressed." The rate in ms at which the LED flashes can be set through a sysfs variable, `toggle_millis`, and the button is "pressed" by setting another sysfs variable `bbb` to 1 and 0.

I decided to get a little fancy so I made sure that if the interval was being changed while the button was depressed the flashing would smoothly adapt to the new rate. This would be particularly useful if you, for example, had an led's flashing rate tied to a varrying signal that changed smoothly. If the rate is changed while the button is flashing, the high resolution timer that flips the state is rescheduled to the time the new interval would have flipped at (or flips immediately if that time has already passed).

In the end, I didn't take the job but at least I know a little about driver development now.