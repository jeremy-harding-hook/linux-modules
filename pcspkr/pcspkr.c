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
#include "../thinkpad_acpi/thinkpad_acpi.h"

MODULE_AUTHOR("Jeremy Harding Hook <jeremyhhook@gmail.com>");
MODULE_DESCRIPTION("PC Speaker beeper pseudodriver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pcspkr");

#define TRUE 1
#define FALSE 0
#define INCLUDE_TESTING_CODE FALSE

static void print_device_details(struct device *device, bool children);

#define MIN_FLASH_MSECS 3000
static int pcspkr_event(struct input_dev *dev, unsigned int type,
		unsigned int code, int value)
{
	/* TODO: figure out why the light is only flashing some of the time and is
	 * so slow to respond. Investigate if there's a better approach.
	 * No, led_set_brightness_sync is not the answer (it locks up the CPU to
	 * predictable results).
	 */
	static int number_of_calls = 0;
	struct led_classdev *power_led = &tpacpi_get_led(0)->led_classdev;
	static unsigned int min_led_off_msecs = 0;
	unsigned int msecs_now = 0;
	unsigned long blink_msecs = 0;
	unsigned long blink_msecs_off = 3000;

	printk(KERN_DEBUG "Starting to beep! This is beep number %d.\n",
			++number_of_calls);
	printk(KERN_DEBUG "Type input: %d\n", type);
	printk(KERN_DEBUG "Code input: %d\n", code);
	printk(KERN_DEBUG "Value input: %d\n", value);

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
		printk(KERN_DEBUG "Turning led on!\n");
		led_set_brightness( power_led, power_led->max_brightness);
		min_led_off_msecs = jiffies_to_msecs(jiffies) + MIN_FLASH_MSECS;
	}
	else
	{
		printk(KERN_DEBUG "Turning led off!\n");
		msecs_now  = jiffies_to_msecs(jiffies);
		if(msecs_now > min_led_off_msecs)
			led_set_brightness(power_led, LED_OFF);
		else
		{
			printk(KERN_DEBUG "Hang on, it's not been on long enough. \
					Blinking.\n");
			blink_msecs = min_led_off_msecs - msecs_now;
			led_blink_set_oneshot(power_led,
					&blink_msecs,
					&blink_msecs_off,
					FALSE);
		}
	}

	printk(KERN_DEBUG "End of beep handling for beep number %d.\n",
			number_of_calls);
	return 0;
}

static int handle_child(struct device *dev, void *data)
{
	struct led_classdev *led;
	printk(KERN_DEBUG "Handling child %s\n", dev->kobj.name);
	print_device_details(dev, TRUE);
	led = devm_of_led_get(dev, 0);
	if(IS_ERR(led))
		printk(KERN_DEBUG "Not an led device.\n");
	else
		printk(KERN_DEBUG "This is an led device!\n");
	printk(KERN_DEBUG "End of child %s\n", dev->kobj.name);
	return 0;
}

static void print_device_details(struct device *device, bool children){
	printk(KERN_DEBUG "kobj name: %s\n", device->kobj.name);
	printk(KERN_DEBUG "Initial device name: %s\n", device->init_name);
	if(device->of_node)
	{
		printk(KERN_DEBUG "of_node has a value!\n");
		printk(KERN_DEBUG "of_node name: %s\n", device->of_node->name);
		printk(KERN_DEBUG "of_node full name: %s\n", device->of_node->
				full_name);
	}
	else
		printk(KERN_DEBUG "of_node is null!\n");
	if(device->fwnode)
		printk(KERN_DEBUG "fwnode has a value!\n");
	else
		printk(KERN_DEBUG "fwnode is null!\n");
	if(device->class){
		printk(KERN_DEBUG "class has a value!\n");
		printk(KERN_DEBUG "class name: %s\n", device->class->name);
	}
	else
		printk(KERN_DEBUG "class is null!\n");
	if(device->bus)
		printk(KERN_DEBUG "bus_type: %s\n", device->bus->name);
	else
		printk(KERN_DEBUG "Unknown bus_type\n");
	if(children)
	{
		printk(KERN_DEBUG "Children:\n");
		device_for_each_child(device, NULL, handle_child);
	}
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

	return 0;
}

static int pcspkr_remove(struct platform_device *dev)
{
	struct input_dev *pcspkr_dev = platform_get_drvdata(dev);

	input_unregister_device(pcspkr_dev);
	/* turn off the speaker */
	pcspkr_event(NULL, EV_SND, SND_BELL, 0);

	return 0;
}

static int pcspkr_suspend(struct device *dev)
{
	pcspkr_event(NULL, EV_SND, SND_BELL, 0);

	return 0;
}

static void pcspkr_shutdown(struct platform_device *dev)
{
	/* turn off the speaker */
	pcspkr_event(NULL, EV_SND, SND_BELL, 0);
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
