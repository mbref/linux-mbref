/*
 * CPU-version specific code
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2006 PetaLogix
 */

#include <linux/init.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/cpu.h>
#include <linux/initrd.h>

#include <asm/cpuinfo.h>
#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/bug.h>
#include <asm/param.h>
#include <asm/delay.h>

static int show_cpuinfo(struct seq_file *m, void *v)
{
	int count = 0;
	char *fpga_family = "Unknown";
	char *cpu_ver = "Unknown";
	int i;

	/* Denormalised to get the fpga family string */
	for (i = 0; family_string_lookup[i].s != NULL; i++) {
		if (cpuinfo.fpga_family_code == family_string_lookup[i].k) {
			fpga_family = (char *)family_string_lookup[i].s;
			break;
		}
	}

	/* Denormalised to get the hw version string */
	for (i = 0; cpu_ver_lookup[i].s != NULL; i++) {
		if (cpuinfo.ver_code == cpu_ver_lookup[i].k) {
			cpu_ver = (char *)cpu_ver_lookup[i].s;
			break;
		}
	}

	count = seq_printf(m,
			"CPU-Family:	MicroBlaze\n"
			"FPGA-Arch:	%s\n"
			"CPU-Ver:	%s\n"
			"CPU-MHz:	%d.%02d\n"
			"BogoMips:	%lu.%02lu\n",
			fpga_family,
			cpu_ver,
			cpuinfo.cpu_clock_freq /
			1000000,
			cpuinfo.cpu_clock_freq %
			1000000,
			loops_per_jiffy / (500000 / HZ),
			(loops_per_jiffy / (5000 / HZ)) % 100);

	count += seq_printf(m,
			"HW-Div:\t\t%s\n"
			"HW-Shift:\t%s\n",
			cpuinfo.use_divider ? "yes" : "no",
			cpuinfo.use_barrel ? "yes" : "no");

	if (cpuinfo.use_icache)
		count += seq_printf(m,
				"Icache:\t\t%ukB\n",
				cpuinfo.icache_size >> 10);
	else
		count += seq_printf(m, "Icache:\t\tno\n");

	if (cpuinfo.use_dcache)
		count += seq_printf(m,
				"Dcache:\t\t%ukB\n",
				cpuinfo.dcache_size >> 10);
	else
		count += seq_printf(m, "Dcache:\t\tno\n");

	count += seq_printf(m,
			"HW-Debug:\t%s\n",
			cpuinfo.hw_debug ? "yes" : "no");

	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	int i = *pos;

	return i < NR_CPUS ? (void *) (i + 1) : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return c_start(m, pos);
}

static void c_stop(struct seq_file *m, void *v)
{
}

struct seq_operations cpuinfo_op = {
	.start = c_start,
	.next = c_next,
	.stop = c_stop,
	.show = show_cpuinfo,
};
