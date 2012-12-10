/*
 *	Intel CPU Microcode Update Driver for Linux
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
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/earlycpio.h>
#include <asm/msr.h>
#include <asm/microcode_intel.h>
#include <asm/processor.h>

struct microcode_intel __initdata *mc_saved_in_initrd[MAX_UCODE_COUNT];
struct mc_saved_data mc_saved_data;

enum ucode_state
generic_load_microcode_early(int cpu, struct microcode_intel **mc_saved_p,
			     unsigned int mc_saved_count,
			     struct ucode_cpu_info *uci)
{
	struct microcode_intel *ucode_ptr, *new_mc = NULL;
	int new_rev = uci->cpu_sig.rev;
	enum ucode_state state = UCODE_OK;
	unsigned int mc_size;
	struct microcode_header_intel *mc_header;
	unsigned int csig = uci->cpu_sig.sig;
	unsigned int cpf = uci->cpu_sig.pf;
	int i;

	for (i = 0; i < mc_saved_count; i++) {
		ucode_ptr = mc_saved_p[i];
		mc_header = (struct microcode_header_intel *)ucode_ptr;
		mc_size = get_totalsize(mc_header);
		if (get_matching_microcode(csig, cpf, ucode_ptr, new_rev)) {
			new_rev = mc_header->rev;
			new_mc  = ucode_ptr;
		}
	}

	if (!new_mc) {
		state = UCODE_NFOUND;
		goto out;
	}

	uci->mc = (struct microcode_intel *)new_mc;
out:
	return state;
}
EXPORT_SYMBOL_GPL(generic_load_microcode_early);

static enum ucode_state __init
load_microcode(struct mc_saved_data *mc_saved_data, int cpu)
{
	struct ucode_cpu_info *uci = mc_saved_data->ucode_cpu_info + cpu;

	return generic_load_microcode_early(cpu, mc_saved_data->mc_saved,
					    mc_saved_data->mc_saved_count, uci);
}

static u8 get_x86_family(unsigned long sig)
{
	u8 x86;

	x86 = (sig >> 8) & 0xf;

	if (x86 == 0xf)
		x86 += (sig >> 20) & 0xff;

	return x86;
}

static u8 get_x86_model(unsigned long sig)
{
	u8 x86, x86_model;

	x86 = get_x86_family(sig);
	x86_model = (sig >> 4) & 0xf;

	if (x86 == 0x6 || x86 == 0xf)
		x86_model += ((sig >> 16) & 0xf) << 4;

	return x86_model;
}

static enum ucode_state
matching_model_microcode(struct microcode_header_intel *mc_header,
			unsigned long sig)
{
	u8 x86, x86_model;
	u8 x86_ucode, x86_model_ucode;

	x86 = get_x86_family(sig);
	x86_model = get_x86_model(sig);

	x86_ucode = get_x86_family(mc_header->sig);
	x86_model_ucode = get_x86_model(mc_header->sig);

	if (x86 != x86_ucode || x86_model != x86_model_ucode)
		return UCODE_ERROR;

	return UCODE_OK;
}

static void
save_microcode(struct mc_saved_data *mc_saved_data,
	       struct microcode_intel **mc_saved_src,
	       unsigned int mc_saved_count)
{
	int i;
	struct microcode_intel **mc_saved_p;

	if (!mc_saved_count)
		return;

	mc_saved_p = vmalloc(mc_saved_count*sizeof(struct microcode_intel *));
	if (!mc_saved_p)
		return;

	for (i = 0; i < mc_saved_count; i++) {
		struct microcode_intel *mc = mc_saved_src[i];
		struct microcode_header_intel *mc_header = &mc->hdr;
		unsigned long mc_size = get_totalsize(mc_header);
		mc_saved_p[i] = vmalloc(mc_size);
		if (mc_saved_src[i])
			memcpy(mc_saved_p[i], mc, mc_size);
	}

	mc_saved_data->mc_saved = mc_saved_p;
}

/*
 * Get microcode matching with BSP's model. Only CPU's with the same model as
 * BSP can stay in the platform.
 */
enum ucode_state
get_matching_model_microcode(int cpu, void *data, size_t size,
			     struct mc_saved_data *mc_saved_data,
			     struct microcode_intel **mc_saved_in_initrd,
			     enum system_states system_state)
{
	u8 *ucode_ptr = data;
	unsigned int leftover = size;
	enum ucode_state state = UCODE_OK;
	unsigned int mc_size;
	struct microcode_header_intel *mc_header;
	struct microcode_intel *mc_saved_tmp[MAX_UCODE_COUNT];
	size_t mc_saved_size;
	size_t mem_size;
	unsigned int mc_saved_count = mc_saved_data->mc_saved_count;
	struct ucode_cpu_info *uci = mc_saved_data->ucode_cpu_info + cpu;
	int found = 0;
	int i;

	if (mc_saved_count) {
		mem_size = mc_saved_count * sizeof(struct microcode_intel *);
		memcpy(mc_saved_tmp, mc_saved_data->mc_saved, mem_size);
	}

	while (leftover) {
		mc_header = (struct microcode_header_intel *)ucode_ptr;

		mc_size = get_totalsize(mc_header);
		if (!mc_size || mc_size > leftover ||
			microcode_sanity_check(ucode_ptr, 0) < 0)
			break;

		leftover -= mc_size;
		if (matching_model_microcode(mc_header, uci->cpu_sig.sig) !=
			 UCODE_OK) {
			ucode_ptr += mc_size;
			continue;
		}

		found = 0;
		for (i = 0; i < mc_saved_count; i++) {
			unsigned int sig, pf;
			unsigned int new_rev;
			struct microcode_header_intel *mc_saved_header =
			     (struct microcode_header_intel *)mc_saved_tmp[i];
			sig = mc_saved_header->sig;
			pf = mc_saved_header->pf;
			new_rev = mc_header->rev;

			if (get_matching_sig(sig, pf, ucode_ptr, new_rev)) {
				found = 1;
				if (update_match_revision(mc_header, new_rev)) {
					/*
					 * Found an older ucode saved before.
					 * Replace the older one with this newer
					 * one.
					 */
					mc_saved_tmp[i] =
					(struct microcode_intel *)ucode_ptr;
					break;
				}
			}
		}
		if (i >= mc_saved_count && !found)
			/*
			 * This ucode is first time discovered in ucode file.
			 * Save it to memory.
			 */
			mc_saved_tmp[mc_saved_count++] =
					 (struct microcode_intel *)ucode_ptr;

		ucode_ptr += mc_size;
	}

	if (leftover) {
		state = UCODE_ERROR;
		goto out;
	}

	if (mc_saved_count == 0) {
		state = UCODE_NFOUND;
		goto out;
	}

	if (system_state == SYSTEM_RUNNING) {
		vfree(mc_saved_data->mc_saved);
		save_microcode(mc_saved_data, mc_saved_tmp, mc_saved_count);
	} else {
		mc_saved_size = sizeof(struct microcode_intel *) *
				mc_saved_count;
		memcpy(mc_saved_in_initrd, mc_saved_tmp, mc_saved_size);
		mc_saved_data->mc_saved = mc_saved_in_initrd;
	}

	mc_saved_data->mc_saved_count = mc_saved_count;
out:
	return state;
}
EXPORT_SYMBOL_GPL(get_matching_model_microcode);

#define native_rdmsr(msr, val1, val2)		\
do {						\
	u64 __val = native_read_msr((msr));	\
	(void)((val1) = (u32)__val);		\
	(void)((val2) = (u32)(__val >> 32));	\
} while (0)

#define native_wrmsr(msr, low, high)		\
	native_write_msr(msr, low, high);

static int __cpuinit collect_cpu_info_early(struct ucode_cpu_info *uci)
{
	unsigned int val[2];
	u8 x86, x86_model;
	struct cpu_signature csig = {0, 0, 0};
	unsigned int eax, ebx, ecx, edx;

	memset(uci, 0, sizeof(*uci));

	eax = 0x00000001;
	ecx = 0;
	native_cpuid(&eax, &ebx, &ecx, &edx);
	csig.sig = eax;

	x86 = get_x86_family(csig.sig);
	x86_model = get_x86_model(csig.sig);

	if ((x86_model >= 5) || (x86 > 6)) {
		/* get processor flags from MSR 0x17 */
		native_rdmsr(MSR_IA32_PLATFORM_ID, val[0], val[1]);
		csig.pf = 1 << ((val[1] >> 18) & 7);
	}

	/* get the current revision from MSR 0x8B */
	native_rdmsr(MSR_IA32_UCODE_REV, val[0], val[1]);

	csig.rev = val[1];

	uci->cpu_sig = csig;
	uci->valid = 1;

	return 0;
}

static __init enum ucode_state
scan_microcode(unsigned long start, unsigned long end,
		struct mc_saved_data *mc_saved_data,
		struct microcode_intel **mc_saved_in_initrd)
{
	unsigned int size = end - start + 1;
	struct cpio_data cd = { 0, 0 };
	char ucode_name[] = "kernel/x86/microcode/GenuineIntel.bin";
	long offset = 0;

	cd = find_cpio_data(ucode_name, (void *)start, size, &offset);
	if (!cd.data)
		return UCODE_ERROR;

	return get_matching_model_microcode(0, cd.data, cd.size, mc_saved_data,
					 mc_saved_in_initrd, SYSTEM_BOOTING);
}

static int __init
apply_microcode_early(struct mc_saved_data *mc_saved_data, int cpu)
{
	struct ucode_cpu_info *uci = mc_saved_data->ucode_cpu_info + cpu;
	struct microcode_intel *mc_intel;
	unsigned int val[2];

	/* We should bind the task to the CPU */
	mc_intel = uci->mc;
	if (mc_intel == NULL)
		return 0;

	/* write microcode via MSR 0x79 */
	native_wrmsr(MSR_IA32_UCODE_WRITE,
	      (unsigned long) mc_intel->bits,
	      (unsigned long) mc_intel->bits >> 16 >> 16);
	native_wrmsr(MSR_IA32_UCODE_REV, 0, 0);

	/* As documented in the SDM: Do a CPUID 1 here */
	sync_core();

	/* get the current revision from MSR 0x8B */
	native_rdmsr(MSR_IA32_UCODE_REV, val[0], val[1]);
	if (val[1] != mc_intel->hdr.rev)
		return -1;

	uci->cpu_sig.rev = val[1];

	return 0;
}

#ifdef CONFIG_X86_32
static void __init map_mc_saved(struct mc_saved_data *mc_saved_data,
				struct microcode_intel **mc_saved_in_initrd)
{
	int i;

	if (mc_saved_data->mc_saved) {
		for (i = 0; i < mc_saved_data->mc_saved_count; i++)
			mc_saved_data->mc_saved[i] =
					 __va(mc_saved_data->mc_saved[i]);

		mc_saved_data->mc_saved = __va(mc_saved_data->mc_saved);
	}

	if (mc_saved_data->ucode_cpu_info->mc)
		mc_saved_data->ucode_cpu_info->mc =
				 __va(mc_saved_data->ucode_cpu_info->mc);
	mc_saved_data->ucode_cpu_info = __va(mc_saved_data->ucode_cpu_info);
}
#else
static inline void __init map_mc_saved(struct mc_saved_data *mc_saved_data,
				struct microcode_intel **mc_saved_in_initrd)
{
}
#endif

void __init save_microcode_in_initrd(struct mc_saved_data *mc_saved_data,
		 struct microcode_intel **mc_saved_in_initrd)
{
	unsigned int count = mc_saved_data->mc_saved_count;

	save_microcode(mc_saved_data, mc_saved_in_initrd, count);
}

static void __init
_load_ucode_intel_bsp(struct mc_saved_data *mc_saved_data,
		      struct microcode_intel **mc_saved_in_initrd,
		      unsigned long initrd_start, unsigned long initrd_end)
{
	int cpu = 0;

#ifdef CONFIG_X86_64
	mc_saved_data->ucode_cpu_info = ucode_cpu_info_early;
#else
	mc_saved_data->ucode_cpu_info =
			(struct ucode_cpu_info *)__pa(ucode_cpu_info_early);
#endif
	collect_cpu_info_early(mc_saved_data->ucode_cpu_info + cpu);
	scan_microcode(initrd_start, initrd_end, mc_saved_data,
		       mc_saved_in_initrd);
	load_microcode(mc_saved_data, cpu);
	apply_microcode_early(mc_saved_data, cpu);
	map_mc_saved(mc_saved_data, mc_saved_in_initrd);
}

void __init
load_ucode_intel_bsp(char *real_mode_data)
{
	u64 ramdisk_image, ramdisk_size, ramdisk_end;
	unsigned long initrd_start, initrd_end;
	struct boot_params *boot_params;

	boot_params = (struct boot_params *)real_mode_data;
	ramdisk_image = boot_params->hdr.ramdisk_image;
	ramdisk_size  = boot_params->hdr.ramdisk_size;

#ifdef CONFIG_X86_64
	ramdisk_end  = PAGE_ALIGN(ramdisk_image + ramdisk_size);
	initrd_start = ramdisk_image + PAGE_OFFSET;
	initrd_end = initrd_start + ramdisk_size;
	_load_ucode_intel_bsp(&mc_saved_data, mc_saved_in_initrd,
			      initrd_start, initrd_end);
#else
	ramdisk_end  = ramdisk_image + ramdisk_size;
	initrd_start = ramdisk_image;
	initrd_end = initrd_start + ramdisk_size;
	_load_ucode_intel_bsp((struct mc_saved_data *)__pa(&mc_saved_data),
			(struct microcode_intel **)__pa(mc_saved_in_initrd),
			initrd_start, initrd_end);
#endif
}

void __cpuinit load_ucode_intel_ap(void)
{
	int cpu = smp_processor_id();

	/*
	 * If BSP doesn't find valid ucode and save it in memory, no need to
	 * update ucode on this AP.
	 */
	if (!mc_saved_data.mc_saved)
		return;

	collect_cpu_info_early(mc_saved_data.ucode_cpu_info + cpu);
	load_microcode(&mc_saved_data, cpu);
	apply_microcode_early(&mc_saved_data, cpu);
}
