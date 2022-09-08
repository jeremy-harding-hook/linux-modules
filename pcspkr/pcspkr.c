// SPDX-License-Identifier: GPL-2.0-only
/*
 *  PC Speaker beeper pseudodriver for Linux on compatible Thinkpad devices
 *
 *  The goal of this pseudodriver is to allow the userspace to call the pcspkr
 *  however they'd like, but rather than making a lound and obnoxious beep in
 *  a library the computer will simply flash the power button LED to give the
 *  user a visual cue. 
 *
 *  This probably works best when the user has the power button LED normally
 *  off, and when it's a piercing bright light like on my machine.
 *
 *  p.s. I really wish they'd go back to nice green and blue indicators, with
 *  the occasional amber.
 *
 * 	This driver is based on pcspkr.c by Vojtech Pavlik <vojtech@ucw.cz>, which
 * 	was distrubuted	under GPL-2.0-only with version 5.18.10 of the Linux kernel.
 * 	I obtained it from https://www.kernel.org/
 *
 *  Copyright (c) 2022 Jeremy Harding Hook
 *  Copyright (c) 2002 Vojtech Pavlik
 *  Copyright (c) 1992 Orest Zborowski
 */

#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i8253.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/timex.h>
#include <linux/io.h>
#include <linux/string.h>
#include <linux/device/class.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include "../thinkpad_acpi/thinkpad_acpi.h"

MODULE_AUTHOR("Jeremy Harding Hook <jeremyhhook@gmail.com>");
MODULE_DESCRIPTION("PC Speaker beeper pseudodriver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pcspkr");

#define TRUE 1
#define FALSE 0
#define INCLUDE_LOGGING FALSE
#define BEEP_DURATION_SECS 1
#define BEEP_DURATION_NANSECS 0
#define MAX_FLASH_ON_MSECS 100
#define MSEC_OFF_FACTOR 100000

static struct hrtimer terminator;
static struct led_classdev *power_led;

static int pcspkr_event(struct input_dev *dev, unsigned int type,
		unsigned int code, int value)
{
	unsigned long blink_msecs_on = MAX_FLASH_ON_MSECS;
	unsigned long blink_msecs_off;
	unsigned int blink_period;
	static ktime_t beep_duration = 0;
#if INCLUDE_LOGGING
	static int number_of_calls = 0;
	printk(KERN_DEBUG "Starting to beep! This is beep number %d.\n",
			++number_of_calls);
	printk(KERN_DEBUG "Type input: %d\n", type);
	printk(KERN_DEBUG "Code input: %d\n", code);
	printk(KERN_DEBUG "Value input: %d\n", value);
#endif
	if(!beep_duration)
		beep_duration = ktime_set(BEEP_DURATION_SECS, BEEP_DURATION_NANSECS);

	if (type != EV_SND)
		return -EINVAL;

	switch (code) {
	case SND_BELL:
		if (value)
			value = 1000;
		break;
	case SND_TONE:
		break;
	default:
		return -EINVAL;
	}

	if (value > 20 && value < 32767)
	{
		blink_period = MSEC_OFF_FACTOR/value;
		if(blink_period < blink_msecs_on << 1)
			blink_msecs_on = blink_period >> 1;
		blink_msecs_off = blink_period - blink_msecs_on;
#if INCLUDE_LOGGING
		printk(KERN_DEBUG "Turning led on!\n");
#endif
		hrtimer_cancel(&terminator);
		led_blink_set(power_led, &blink_msecs_on, &blink_msecs_off);
		hrtimer_start(&terminator, beep_duration, HRTIMER_MODE_REL);
	}
#if INCLUDE_LOGGING
	else
	{
		printk(KERN_DEBUG "Ignoring beep end.\n");
	}

	printk(KERN_DEBUG "End of beep handling for beep number %d.\n",
			number_of_calls);
#endif
	return 0;
}

static enum hrtimer_restart terminate_flasher(struct hrtimer *terminator)
{
	led_set_brightness(power_led, LED_OFF);
	return HRTIMER_NORESTART;
}

static int pcspkr_probe(struct platform_device *dev)
{
	struct input_dev *pcspkr_dev;
	int err;

	pcspkr_dev = input_allocate_device();
	if (!pcspkr_dev)
		return -ENOMEM;

	pcspkr_dev->name = "PC Speaker";
	pcspkr_dev->phys = "isa0061/input0";
	pcspkr_dev->id.bustype = BUS_ISA;
	pcspkr_dev->id.vendor = 0x001f;
	pcspkr_dev->id.product = 0x0001;
	pcspkr_dev->id.version = 0x0100;
	pcspkr_dev->dev.parent = &dev->dev;

	pcspkr_dev->evbit[0] = BIT_MASK(EV_SND);
	pcspkr_dev->sndbit[0] = BIT_MASK(SND_BELL) | BIT_MASK(SND_TONE);
	pcspkr_dev->event = pcspkr_event;

	err = input_register_device(pcspkr_dev);
	if (err) {
		input_free_device(pcspkr_dev);
		return err;
	}

	platform_set_drvdata(dev, pcspkr_dev);
	hrtimer_init(&terminator, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	terminator.function = terminate_flasher;
	power_led = &tpacpi_get_led(0)->led_classdev;
	return 0;
}

static int pcspkr_remove(struct platform_device *dev)
{
	struct input_dev *pcspkr_dev = platform_get_drvdata(dev);

	input_unregister_device(pcspkr_dev);
	/* stop flashing */
	hrtimer_cancel(&terminator);
	terminate_flasher(NULL);

	return 0;
}

static int pcspkr_suspend(struct device *dev)
{
	/* stop flashing */
	hrtimer_cancel(&terminator);
	terminate_flasher(NULL);

	return 0;
}

static void pcspkr_shutdown(struct platform_device *dev)
{
	/* stop flashing */
	hrtimer_cancel(&terminator);
	terminate_flasher(NULL);
}

static const struct dev_pm_ops pcspkr_pm_ops = {
	.suspend = pcspkr_suspend,
};

static struct platform_driver pcspkr_platform_driver = {
	.driver		= {
		.name	= "pcspkr",
		.pm	= &pcspkr_pm_ops,
	},
	.probe		= pcspkr_probe,
	.remove		= pcspkr_remove,
	.shutdown	= pcspkr_shutdown,
};
module_platform_driver(pcspkr_platform_driver);
