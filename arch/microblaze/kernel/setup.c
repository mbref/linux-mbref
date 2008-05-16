/*
 * arch/microblaze/kernel/setup.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2008 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2006 Atmark Techno, Inc.
 */

#include <linux/init.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/cpu.h>
#include <linux/initrd.h>
#include <linux/console.h>

#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/bug.h>
#include <asm/param.h>
#include <asm/cache.h>
#include <asm/cacheflush.h>
#include <asm/entry.h>
#include <asm/cpuinfo.h>

#include <asm/system.h>
#include <asm/prom.h>
#include <asm/pgtable.h>

DEFINE_PER_CPU(unsigned int, KSP);	/* Saved kernel stack pointer */
DEFINE_PER_CPU(unsigned int, KM);	/* Kernel/user mode */
DEFINE_PER_CPU(unsigned int, ENTRY_SP);	/* Saved SP on kernel entry */
DEFINE_PER_CPU(unsigned int, R11_SAVE);	/* Temp variable for entry */
DEFINE_PER_CPU(unsigned int, CURRENT_SAVE);	/* Saved current pointer */

int have_of = 1;
char command_line[COMMAND_LINE_SIZE];
static char default_command_line[COMMAND_LINE_SIZE] __initdata = CONFIG_CMDLINE;

void __init setup_arch(char **cmdline_p)
{
	console_verbose();

	unflatten_device_tree();
	/* NOTE I think that this function is not necessary to call */
	/* irq_early_init(); */
	setup_cpuinfo();

	__invalidate_icache_all();
	__enable_icache();

	__invalidate_dcache_all();
	__enable_dcache();

	panic_timeout = 120;

	setup_memory();
	paging_init();
#ifdef CONFIG_HACK
	printk(KERN_NOTICE "HACK function enable\n");
#else
	printk(KERN_NOTICE "HACK function disable\n");
#endif

#ifdef CONFIG_VT
#if defined(CONFIG_XILINX_CONSOLE)
	conswitchp = &xil_con;
#elif defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
#endif
}

#ifdef CONFIG_MTD_UCLINUX
/* Handle both romfs and cramfs types, without generating unnecessary
 code (ie no point checking for CRAMFS if it's not even enabled) */
inline unsigned get_romfs_len(unsigned *addr)
{
#ifdef CONFIG_ROMFS_FS
	if (memcmp(&addr[0], "-rom1fs-", 8) == 0) /* romfs */
		return be32_to_cpu(addr[2]);
#endif

#ifdef CONFIG_CRAMFS
	if (addr[0] == le32_to_cpu(0x28cd3d45)) /* cramfs */
		return le32_to_cpu(addr[1]);
#endif
	return 0;
}
#endif	/* CONFIG_MTD_UCLINUX_EBSS */

void __init machine_early_init(const char *cmdline, unsigned int ram,
		unsigned int fdt)
{
	unsigned long *src, *dst = (unsigned long *)0x0;
	early_printk("Ramdisk addr 0x%08x, FDT 0x%08x\n", ram, fdt);

#ifdef CONFIG_MTD_UCLINUX
	{
		int size;
		unsigned int romfs_base;
		romfs_base = (ram ? ram : (unsigned int)&__init_end);
		/* if CONFIG_MTD_UCLINUX_EBSS is defined, assume ROMFS is at the
		 * end of kernel, which is ROMFS_LOCATION defined above. */
		size = PAGE_ALIGN(get_romfs_len((unsigned *)romfs_base));
		early_printk("Found romfs @ 0x%08x (0x%08x)\n",
				romfs_base, size);
		early_printk("#### klimit %p ####\n", klimit);
		BUG_ON(size < 0); /* What else can we do? */

		/* Use memmove to handle likely case of memory overlap */
		early_printk("Moving 0x%08x bytes from 0x%08x to 0x%08x\n",
			size, romfs_base, &_ebss);
		memmove(&_ebss, (int *)romfs_base, size);

		/* update klimit */
		klimit += PAGE_ALIGN(size);
		early_printk("New klimit: 0x%08x\n", klimit);
	}
#endif

	memset(__bss_start, 0, __bss_stop-__bss_start);
	memset(_ssbss, 0, _esbss-_ssbss);

	printk(KERN_NOTICE "Found FDT at 0x%08x\n", fdt);
	early_init_devtree((void *)fdt);

	/* Copy command line passed from bootloader, or use default
	 if none provided, or forced */
#ifndef CONFIG_CMDLINE_FORCE
	if (cmdline && cmdline[0] != '\0')
		strlcpy(command_line, cmdline, COMMAND_LINE_SIZE);
	else
#endif
		strlcpy(command_line, default_command_line, COMMAND_LINE_SIZE);

	for (src = __ivt_start; src < __ivt_end; src++, dst++)
		*dst = *src;

	/* Initialize global data */
	per_cpu(KM, 0) = 0x1;	/* We start in kernel mode */
	per_cpu(CURRENT_SAVE, 0) = (unsigned long)current;

}

void machine_restart(char *cmd)
{
	printk(KERN_NOTICE "Machine restart...\n");
	dump_stack();
	while (1)
		;
}

void machine_shutdown(void)
{
	printk(KERN_NOTICE "Machine shutdown...\n");
	while (1)
		;
}

void machine_halt(void)
{
	printk(KERN_NOTICE "Machine halt...\n");
	while (1)
		;
}

void machine_power_off(void)
{
	printk(KERN_NOTICE "Machine power off...\n");
	while (1)
		;
}
