/*
 * Copyright (C) 2007-2008 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_MICROBLAZE_SETUP_H
#define _ASM_MICROBLAZE_SETUP_H

#include <linux/init.h>

extern unsigned int boot_cpuid;
extern int have_of;

#define COMMAND_LINE_SIZE	256
extern char cmd_line[COMMAND_LINE_SIZE];

int __init setup_early_printk(char *opt);
void early_printk(const char *fmt, ...);

#ifdef CONFIG_HEART_BEAT
void heartbeat(void);
#endif

void system_timer_init(void);
void time_init(void);
unsigned long long sched_clock(void);

void __init setup_memory(void);
void __init machine_early_init(const char *cmdline, unsigned int ram,
			unsigned int fdt);

void machine_restart(char *cmd);
void machine_shutdown(void);
void machine_halt(void);
void machine_power_off(void);

#endif /* _ASM_MICROBLAZE_SETUP_H */
