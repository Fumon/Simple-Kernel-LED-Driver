/*
 *  Initialize and function as the GPIO controller.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/init.h>

// Required for hrtimer
MODULE_LICENSE("GPL");

#define MS_TO_NS(x)	(x * 1E6L)

// Jiffies to wait before repolling the button
#define BUTTON_POLL_RATE 2000

// Time toggling LED in milliseconds.
// Default to 500 ms.
static ulong toggle_millis = 1000;
// Register with sysfs.
module_param(toggle_millis, ulong, 0644);

static bool bbb = 0;
module_param(bbb, bool, 0644);

// Rememberd when high res timer is started.
static ulong toggle_millis_cached = 0;

// High resolution led timer.
static struct hrtimer led_toggle_timer;

// Low resolution button polling.
static struct timer_list button_polling_timer;

// State of flashing.
static volatile int flashing = 0;

// State bit for led. 
static volatile int led_on = 0;

// High res timer callback for toggling led.
enum hrtimer_restart led_toggle_cb(struct hrtimer *timer) {
  ktime_t now;
  ktime_t interval;
  // Cache the time as the callback begins.
  now = hrtimer_cb_get_time(timer);

  if(led_on) {
    printk("- -\n");
  } else {
    printk("---\n");
  }
  led_on ^= 1;

  if(toggle_millis_cached != toggle_millis) {
    printk("*+*+*+*+*+*+*+*+HR TIMER Detected interval change\n");
    toggle_millis_cached = toggle_millis;
  }
  // Schedule.
  interval = ktime_set(0, MS_TO_NS(toggle_millis));

  // Reschedule timer.
  hrtimer_forward(&led_toggle_timer, now, interval);

	return HRTIMER_RESTART;
}

// Button polling timer callback.
static void button_poll_cb(unsigned long data) {
  ktime_t ktime;
  printk("+++Hi there\n");

  if(bbb) {
    if(!flashing) {
      printk("Starting hrtimer.\n");
      flashing = 1;

      // Turn the LED on
      /*
      (*GPIOWRITE) ^= LED_MASK;
      */
      led_on = 1;

      // Start highres timer.
      toggle_millis_cached = toggle_millis;
      ktime = ktime_set(0, MS_TO_NS(toggle_millis));
      hrtimer_init(&led_toggle_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
      led_toggle_timer.function = &led_toggle_cb;
      hrtimer_start(&led_toggle_timer, ktime, HRTIMER_MODE_REL);
    }
	} else if (flashing) {
		// If we are currently flashing, stop flashing and stop timers.
    printk("Cancelling hrtimer.\n");
		flashing = 0;
		// Cancel hrtimer
    hrtimer_cancel(&led_toggle_timer);

		// Turn off the led if it was on.
		if(led_on) {
			//(*GPIOWRITE) ^= LED_MASK;
			led_on = 0;
		}
    led_on=0;

	}
  // Set timer to poll
  mod_timer(&button_polling_timer, jiffies + msecs_to_jiffies(BUTTON_POLL_RATE));
}

static int __init setup_gpio(void) {
  printk("Life begins\n");

	// Register button polling timer
	setup_timer(&button_polling_timer, button_poll_cb, 0);
  // Set timer to poll
  mod_timer(&button_polling_timer, jiffies + msecs_to_jiffies(BUTTON_POLL_RATE));

	return 0;
}

static void __exit destroy_module(void) {
	// Stop all timers.
  printk("I'm dead. Blarg.\n");
  hrtimer_cancel(&led_toggle_timer);
  del_timer(&button_polling_timer);
}

module_init(setup_gpio);
module_exit(destroy_module);
