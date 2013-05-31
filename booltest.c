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

// Forward declaration
struct kernel_param_ops sysfscb;

// Time toggling LED in milliseconds.
// Default to 500 ms.
static ulong toggle_millis = 1000;
// Register with sysfs.
module_param_cb(toggle_millis, &sysfscb, &toggle_millis, 0644);
__MODULE_PARM_TYPE(toggle_millis, "ulong");

static bool bbb = 0;
module_param(bbb, bool, 0644);

// Rememberd when high res timer is started.
static ulong toggle_millis_cached = toggle_millis;

// High resolution led timer.
static struct hrtimer led_toggle_timer;

// Low resolution button polling.
static struct timer_list button_polling_timer;

// State of flashing.
static volatile int flashing = 0;

// State bit for led. 
static volatile int led_on = 0;

// High res timer callback for toggling led.
enum hrtimer_restart led_toggle_cb(struct hrtimer *timer)
{
  ktime_t now;
  ktime_t interval;

  // Cache the time as the callback begins.
  now = hrtimer_cb_get_time(timer);

  led_on ^= 1;

  // Schedule.
  interval = ktime_set(0, MS_TO_NS(toggle_millis));

  // Reschedule timer.
  hrtimer_forward(&led_toggle_timer, now, interval);

	return HRTIMER_RESTART;
}

// Button polling timer callback.
static void button_poll_cb(unsigned long data)
{
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

// Callback for sysfs input. 
// Reacts dynamically to interval updates for smooth blinking speed.
int hrtimerchange(const char *val, const struct kernel_param *kp) 
{
  int ret;
  ktime_t oldinterval;
  ktime_t newinterval;
  ktime_t elapsedtime;
  ktime_t diffperiod;

  // Use standard parameter function.
  ret = param_set_ulong(val, kp);
  if(ret != 0)
    return ret;

  // If we make it this far, the value has been updated.

  // Make sure we don't turn on the timer if we're not already flashing.
  if(flashing) {
    oldinterval = ktime_set(0, MS_TO_NS(toggle_millis_cached));
    newinterval = ktime_set(0, MS_TO_NS(toggle_millis));
    // Calculate how long until the next callback should occur.
    elapsedtime = ktime_sub(oldinterval, hrtimer_get_remaining(&led_toggle_timer));
    diffperiod = ktime_sub(newinterval, elapsedtime);
    // Cache new value.
    toggle_millis_cached = toggle_millis;

    // Now do scheduling of the next expiry.
    hrtimer_cancel(&led_toggle_timer);
    // If time is less than 0, set next expiry to be 1ns in the future.
    if(ktime_to_ns(diffperiod) < 0) {
      diffperiod = ktime_set(0, 1);
    }

    // Schedule timer to expire in remaining time to new interval
    hrtimer_start(&led_toggle_timer, diffperiod, HRTIMER_MODE_REL);
  } else {
    // Cache new value.
    toggle_millis_cached = toggle_millis;
  }
  return 0;
}

struct kernel_param_ops sysfscb = {
  .set = hrtimerchange,
  .get = param_get_ulong,
};        

static int __init setup_gpio(void)
{
	// Register button polling timer
	setup_timer(&button_polling_timer, button_poll_cb, 0);
  // Set timer to poll
  mod_timer(&button_polling_timer, jiffies + msecs_to_jiffies(BUTTON_POLL_RATE));

	return 0;
}

static void __exit destroy_module(void)
{
	// Stop all timers.
  hrtimer_cancel(&led_toggle_timer);
  del_timer(&button_polling_timer);
}

module_init(setup_gpio);
module_exit(destroy_module);
