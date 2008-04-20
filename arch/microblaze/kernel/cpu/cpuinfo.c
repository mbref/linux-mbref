/*
 * arch/microblaze/kernel/cpu/cpuinfo.c
 *
 * Copyright (C) 2007-2008 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2007 John Williams <john.williams@petalogix.com>
 * Copyright (C) 2007 PetaLogix
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/autoconf.h>
#include <asm/cpuinfo.h>
#include <asm/pvr.h>

const struct cpu_ver_key cpu_ver_lookup[] = {
	/* These key value are as per MBV field in PVR0 */
	{"5.00.a", 0x01},
	{"5.00.b", 0x02},
	{"5.00.c", 0x03},
	{"6.00.a", 0x04},
	{"6.00.b", 0x06},
	{"7.00.a", 0x05},
	{"7.00.b", 0x07},
	{"7.10.a", 0x08},
	/* FIXME There is no keycode defined in MBV for these versions */
	{"2.10.a", 0x10},
	{"3.00.a", 0x20},
	{"4.00.a", 0x30},
	{"4.00.b", 0x40},
	{NULL, 0},
};

/*
 * FIXME Not sure if the actual key is defined by Xilinx in the PVR
 */
const struct family_string_key family_string_lookup[] = {
	{"virtex2", 0x4},
	{"virtex2pro", 0x5},
	{"spartan3", 0x6},
	{"virtex4", 0x7},
	{"virtex5", 0x8},
	{"spartan3e", 0x9},
	{"spartan3a", 0xa},
	{"spartan3an", 0xb},
	{"spartan3adsp", 0xc},
	/* FIXME There is no key code defined for spartan2 */
	{"spartan2", 0xf0},
	{NULL, 0},
};

struct cpuinfo cpuinfo;

void __init setup_cpuinfo(void)
{
	struct device_node *cpu = NULL;

	cpu = (struct device_node *) of_find_node_by_type(NULL, "cpu");
	if (!cpu)
		printk(KERN_ERR "You don't have cpu!!!\n");

	printk(KERN_INFO "%s: initialising\n", __FUNCTION__);

	switch (cpu_has_pvr()) {
	case 0:
		printk(KERN_WARNING
			"%s: No PVR support. Using static CPU info from FDT\n",
			__FUNCTION__);
			set_cpuinfo_static(&cpuinfo, cpu);
		break;
#if 0
	case 1:
		set_cpuinfo_pvr_partial(cpuinfo);
		break;
/* FIXME I found weird behavior with MB 7 */
	case 2:
		printk(KERN_INFO "%s: Using full CPU PVR support\n",
			__FUNCTION__);
		set_cpuinfo_pvr_full(&cpuinfo);
		cpuinfo.cpu_clock_freq = fcpu(cpu, "timebase-frequency");
		break;
#endif
	default:
		WARN_ON(1);
		set_cpuinfo_static(&cpuinfo, cpu);
	}
}
