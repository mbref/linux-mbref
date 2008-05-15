/*
 * Copyright (C) 2008 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/hrtimer.h>
#include <asm/bug.h>
#include <asm/setup.h>

void time_init(void)
{
	system_timer_init();
}

unsigned long long sched_clock(void)
{
	return (unsigned long long)jiffies * (1000000000 / HZ);
}
