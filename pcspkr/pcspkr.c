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
 * 	Largely due to my unfamiliarity with creating drivers, this is loosely
 * 	based on pcspkr.c by Vojtech Pavlik <vojtech@ucw.cz>, which was distrubuted
 * 	under GPL-2.0-only with version 5.18.10 of the Linux kernel. I obtained it 
 * 	from https://www.kernel.org/
 *
 * 	Because of the small size and large differences in intent and functionality,
 * 	I will provide this source directly rather than a patch file unless
 * 	otherwise requested.
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

#if INCLUDE_TESTING_CODE
static void get_led_device(struct class *class);
static int class_match_name(struct class *class, const void *name);
static struct device *find_led_dev(const char *name);
#endif

static int pcspkr_event(struct input_dev *dev, unsigned int type,
		unsigned int code, int value)
{
	/* TODO: clean up all the stuff not to be compiled, fix license/readme at
	 * top, actually flash power led (index=0).
	 * Additionally, I need to decide on a mapping scheme from Hz to flash.
	 * Normally, it seems a beep is achieved by calling a tone with a given
	 * pitch (500 Hz in XTerm, 750 Hz in my native console) then stopping with
	 * a value of zero sent first to the tone and then to the bell (hence 3
	 * calls per beep). For now maybe just a pitch-independent solution of
	 * turning the led on when it's a pitch, and off otherwise?
	 */
	static int number_of_calls = 0;
	struct device *device;
	printk(KERN_DEBUG "Starting to beep! This is beep number %d.\n",
			++number_of_calls);
	printk(KERN_DEBUG "Type input: %d\n", type);
	printk(KERN_DEBUG "Code input: %d\n", code);
	printk(KERN_DEBUG "Value input: %d\n", value);

#if INCLUDE_TESTING_CODE
	if(!dev){
		printk(KERN_DEBUG "input_dev NULL, so just returning 0\n");
		return 0;
	}

	printk(KERN_DEBUG "Input device name: %s\n", dev->name);
	device = &dev->dev;
	do
	{
		print_device_details(device, FALSE);
		if(class_match_name(device->class, "input"))
			get_led_device(device->class);
		printk(KERN_DEBUG "Looking at new parent...");
		device = device->parent;
	}while(device);
#else
	unsigned int count = 0;
	unsigned long flags;

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
		count = PIT_TICK_RATE / value;
#if FALSE
	raw_spin_lock_irqsave(&i8253_lock, flags);

	if (count) {
		/* set command for counter 2, 2 byte write */
		outb_p(0xB6, 0x43);
		/* select desired HZ */
		outb_p(count & 0xff, 0x42);
		outb((count >> 8) & 0xff, 0x42);
		/* enable counter 2 */
		outb_p(inb_p(0x61) | 3, 0x61);
	} else {
		/* disable counter 2 */
		outb(inb_p(0x61) & 0xFC, 0x61);
	}

	raw_spin_unlock_irqrestore(&i8253_lock, flags);
#endif
#endif
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

#if INCLUDE_TESTING_CODE
static void get_led_device(struct class *class){
	// Get the input9 from class "input"
	// TODO: Somehow check all the divices for some other identifier (input9
	// isn't constant)
	int i = 0;
	int digits = 1;
	int sacrifice;
	struct device *device;
	char *name;
	struct led_classdev *led;
	/*do
	  {
	  name = kmalloc(sizeof(char) * (5 + digits), GFP_KERNEL);
	  if(!name)
	  {
	  printk(KERN_WARNING "Out of memory! Aborting search for input "
	  "devices.\n");
	  return;
	  }
	  sprintf(name, "input%d", i);
	  device = class_find_device(class, NULL, name,
	  device_match_name);	
	  if(device)
	  {	
	  printk(KERN_DEBUG "Found input%d device!\n", i);
	  print_device_details(device);
	  led = devm_of_led_get(device, 0);
	  if(IS_ERR(led))
	  printk(KERN_DEBUG "Not an led device.\n");
	  else
	  printk(KERN_DEBUG "This is an led device!\n");
	// TODO: try using bus type to get the correct device?
	put_device(device);
	}
	else
	printk(KERN_DEBUG "Failed to find input%d.\n", i);
	kfree(name);
	i++;
	sacrifice = i;
	digits = 0;
	while(sacrifice != 0){
	sacrifice = sacrifice/10;
	digits++;
	}
	}while(i < 100);*/

	printk(KERN_DEBUG "Trying to use the bus to get the right device...");
	device  = find_led_dev("thinkpad_acpi");
	//device = find_led_dev("tpacpi::power");
	if(device)
	{
		printk(KERN_DEBUG "Device found!!!");
		led = devm_of_led_get(device, 0);
		if(IS_ERR(led))
			printk(KERN_DEBUG "Not an led device.\n");
		else
			printk(KERN_DEBUG "This is an led device!\n");
		print_device_details(device, TRUE);
		put_device(device);
	}
	else
		printk(KERN_DEBUG "Device not found");
}

static int custom_match_dev(struct device *dev, void *data)
{
	/* not sure if this is needed or useful... */
	const char *name = data;
	return sysfs_streq(name, dev->of_node->name);
}

static int class_match_name(struct class *class, const void *name){
	if(!class)
		return 0;
	return sysfs_streq(class->name, name);
}

static struct device *find_led_dev(const char *name)
{
	return bus_find_device(&platform_bus_type, NULL, name, device_match_name);
}
#endif

static int pcspkr_probe(struct platform_device *dev)
{
	// Add something saving the device for the LED
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
