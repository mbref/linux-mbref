/*
 *	X86 CPU microcode early update for Linux
 *
 *	Copyright (C) 2012 Fenghua Yu <fenghua.yu@intel.com>
 *			   H Peter Anvin" <hpa@zytor.com>
 *
 *	This driver allows to early upgrade microcode on Intel processors
 *	belonging to IA-32 family - PentiumPro, Pentium II,
 *	Pentium III, Xeon, Pentium 4, etc.
 *
 *	Reference: Section 9.11 of Volume 3, IA-32 Intel Architecture
 *	Software Developer's Manual.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/mm.h>
#include <asm/microcode_intel.h>
#include <asm/processor.h>

struct ucode_cpu_info	ucode_cpu_info_early[NR_CPUS];
EXPORT_SYMBOL_GPL(ucode_cpu_info_early);

static inline int __init x86_vendor(void)
{
	unsigned int eax = 0x00000000;
	char x86_vendor_id[16];
	int i;
	struct {
		char x86_vendor_id[16];
		__u8 x86_vendor;
	} cpu_vendor_table[] = {
		{ "GenuineIntel", X86_VENDOR_INTEL },
		{ "AuthenticAMD", X86_VENDOR_AMD },
	};

	memset(x86_vendor_id, 0, ARRAY_SIZE(x86_vendor_id));
	/* Get vendor name */
	native_cpuid(&eax,
		(unsigned int *)&x86_vendor_id[0],
		(unsigned int *)&x86_vendor_id[8],
		(unsigned int *)&x86_vendor_id[4]);

	for (i = 0; i < ARRAY_SIZE(cpu_vendor_table); i++) {
		if (!strcmp(x86_vendor_id, cpu_vendor_table[i].x86_vendor_id))
			return cpu_vendor_table[i].x86_vendor;
	}

	return X86_VENDOR_UNKNOWN;
}


void __init load_ucode_bsp(char *real_mode_data)
{
	/*
	 * boot_cpu_data is not setup yet in this early phase.
	 * So we get vendor information directly through cpuid.
	 */
	if (x86_vendor() == X86_VENDOR_INTEL)
		load_ucode_intel_bsp(real_mode_data);
}

void __cpuinit load_ucode_ap(void)
{
	if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL)
		load_ucode_intel_ap();
}
