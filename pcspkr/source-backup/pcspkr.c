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
 *  Copyright (c) 2022 Jeremy Harding Hook
 */


#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i8253.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/timex.h>
#include <linux/io.h>

MODULE_AUTHOR("Jeremy Harding Hook <jeremyhhook@gmail.com>");
MODULE_DESCRIPTION("PC Speaker beeper pseudodriver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pcspkr");
