/*
 * include/asm-microblaze/cpuinfo.h
 *
 * Generic support for queying CPU info
 *
 * Copyright (C) 2007-2008 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2007 John Williams <john.williams@petalogix.com>
 * Copyright (C) 2007 PetaLogix
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file COPYING in the main directory of this
 * archive for more details.
 *
 */

#ifndef _ASM_MICROBLAZE_CPUINFO_H
#define _ASM_MICROBLAZE_CPUINFO_H

#include <asm/prom.h>

extern unsigned long loops_per_jiffy;

/* CPU Version and FPGA Family code conversion table type */
struct cpu_ver_key {
	const char *s;
	const unsigned k;
};

extern const struct cpu_ver_key cpu_ver_lookup[];

struct family_string_key {
	const char *s;
	const unsigned k;
};

extern const struct family_string_key family_string_lookup[];

struct cpuinfo {
	/* Core CPU configuration */
	int use_barrel;
	int use_divider;
	int use_mult;
	int use_fpu;
	int use_exception;
	int use_mul_64;
	int use_msr_instr;
	int use_pcmp_instr;

	int ver_code;

	/* CPU caches */
	int use_icache;
	int icache_tagbits;
	int icache_write;
	int icache_line;
	int icache_size;
	unsigned long icache_base;
	unsigned long icache_high;

	int use_dcache;
	int dcache_tagbits;
	int dcache_write;
	int dcache_line;
	int dcache_size;
	unsigned long dcache_base;
	unsigned long dcache_high;

	/* Bus connections */
	int use_dopb;
	int use_iopb;
	int use_dlmb;
	int use_ilmb;
	int num_fsl;

	/* CPU interrupt line info */
	int irq_edge;
	int irq_positive;

	int area_optimised;

	/* HW support for CPU exceptions */
	int opcode_0_illegal;
	int exc_unaligned;
	int exc_ill_opcode;
	int exc_iopb;
	int exc_dopb;
	int exc_div_zero;
	int exc_fpu;

	/* HW debug support */
	int hw_debug;
	int num_pc_brk;
	int num_rd_brk;
	int num_wr_brk;
	int cpu_clock_freq;

	/* FPGA family */
	int fpga_family_code;
};

extern struct cpuinfo cpuinfo;

/* fwd declarations of the various CPUinfo populators */
void setup_cpuinfo(void);

void set_cpuinfo_static(struct cpuinfo *ci, struct device_node *cpu);
void set_cpuinfo_pvr_partial(struct cpuinfo *ci);
void set_cpuinfo_pvr_full(struct cpuinfo *ci);

static inline unsigned int fcpu(struct device_node *cpu, char *n)
{
	int *val;
	return ((val = (int *) of_get_property(cpu, n, NULL)) ? *val : 0);
}

#endif /* _ASM_MICROBLAZE_CPUINFO_H */
