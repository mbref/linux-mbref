/*
 * ml605.c
 * Copyright (c) 2010 Li-Pro.Net, Stephan Linz <linz@li-pro.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * This code was implemented by Mocean Laboratories AB when porting linux
 * to the automotive development board Russellville. The copyright holder
 * as seen in the header is Intel corporation.
 * Mocean Laboratories forked off the GNU/Linux platform work into a
 * separate company called Pelagicore AB, which commited the code to the
 * kernel.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-xiic.h>

#define DRIVER_NAME "ml605"
#define DRV_VERSION "0.0.1"

static struct xiic_i2c_platform_data ml605_xiic_platform_data = {
	.devices = NULL,
	.num_devices = 0,
};

static struct resource ml605_xiic_resources[] = {
	{
		.start  = 0x81640000,
		.end    = 0x8164ffff,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = 9,
		.end    = 9,
		.flags  = IORESOURCE_IRQ,
	},
};


MODULE_ALIAS("platform:"DRIVER_NAME);

static struct platform_device ml605_xiic_device = {
	.name		= "xiic-i2c",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ml605_xiic_resources),
	.resource	= ml605_xiic_resources,
	.dev = {
		.platform_data = &ml605_xiic_platform_data,
	},
};

static int __init ml605_init(void)
{
	int err;

	err = platform_device_register(&ml605_xiic_device);
	if (err < 0) {
		printk(KERN_ERR
			"Failed to register MFD driver für %s device.\n",
			ml605_xiic_device.name);
		return -ENODEV;
	}

	printk(KERN_INFO "Driver for %s has been successfully registered.\n",
		DRIVER_NAME);

	return 0;
}

static void __exit ml605_exit(void)
{
	platform_device_unregister(&ml605_xiic_device);

	printk(KERN_INFO "Driver for %s has been successfully unregistered.\n",
		ml605_xiic_device.name);
}

module_init(ml605_init);
module_exit(ml605_exit);

MODULE_AUTHOR("Stephan Linz <linz@li-pro.net>");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL v2");
