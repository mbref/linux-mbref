/*
 * arch/microblaze/mm/init.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007-2008 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2006 Atmark Techno, Inc.
 */

#undef DEBUG

#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/mm.h>
#include "../../../mm/internal.h"
#include <linux/swap.h>
#include <linux/bootmem.h>
#include <linux/pfn.h>
#include <linux/lmb.h>

#include <asm/sections.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/pgtable.h>

char *klimit = _end;
static unsigned int memory_start;
static unsigned int memory_end;

unsigned int __page_offset;
/* EXPORT_SYMBOL(__page_offset); */

void __init setup_memory(void)
{
	int i;
	unsigned int start, end;
	unsigned long map_size;
	unsigned long start_pfn = 0;
	unsigned long end_pfn = 0;

	/* Find main memory where is the kernel */
	for (i = 0; i < lmb.memory.cnt; i++) {
		start_pfn = lmb.memory.region[i].base >> PAGE_SHIFT;
		end_pfn = start_pfn + lmb_size_pages(&lmb.memory, i);
		if ((start_pfn <= (((int)_text) >> PAGE_SHIFT)) &&
			(((int)_text >> PAGE_SHIFT) <= end_pfn)) {
			memory_end = (end_pfn << PAGE_SHIFT) - 1;
			PAGE_OFFSET = memory_start = start_pfn << PAGE_SHIFT;
			pr_debug("%s: Main mem: 0x%x-0x%x\n", __func__,
				memory_start, memory_end);
			break;
		}
	}
	/*
	 * start_pfn - start page - starting point
	 * end_pfn - first unused page
	 * memory_start - base physical address of main memory
	 * memory_end - end physical address of main memory
	 * PAGE_OFFSET - moving of first page
	 *
	 * Kernel:
	 * start: base phys address of kernel - page align
	 * end: base phys address of kernel - page align
	 *
	 * min_low_pfn - the first page (mm/bootmem.c - node_boot_start)
	 * max_low_pfn
	 * max_mapnr - the first unused page (mm/bootmem.c - node_low_pfn)
	 * num_physpages - number of all pages
	 *
	 */

	/* reservation of region where is the kernel */
	start = PFN_DOWN((int)_text) << PAGE_SHIFT;
	end = PAGE_ALIGN((unsigned long)klimit);
	lmb_reserve(start, end - start);
	pr_debug("%s: kernel addr 0x%08x-0x%08x\n", __func__, start, end);

	/* calculate free pages, etc. */
	min_low_pfn = PFN_UP(start_pfn << PAGE_SHIFT);
	max_mapnr = PFN_DOWN((end_pfn << PAGE_SHIFT));
	max_low_pfn = max_mapnr - min_low_pfn;
	num_physpages = max_mapnr - min_low_pfn + 1;
	printk(KERN_INFO "%s: max_mapnr: %#lx\n", __func__, max_mapnr);
	printk(KERN_INFO "%s: min_low_pfn: %#lx\n", __func__, min_low_pfn);
	printk(KERN_INFO "%s: max_low_pfn: %#lx\n", __func__, max_low_pfn);

	/* add place for data pages */
	map_size = init_bootmem_node(NODE_DATA(0), PFN_UP(end),
			min_low_pfn, max_mapnr);
	lmb_reserve(PFN_UP(end) << PAGE_SHIFT, map_size);

	/* free bootmem is whole main memory */
	free_bootmem(start_pfn << PAGE_SHIFT,
			((end_pfn - start_pfn) << PAGE_SHIFT) - 1);

	/* reserve allocate blocks */
	for (i = 0; i < lmb.reserved.cnt; i++) {
		pr_debug("reserved %d - 0x%08x-0x%08x\n", i,
			(u32) lmb.reserved.region[i].base,
			(u32) lmb_size_bytes(&lmb.reserved, i));
		reserve_bootmem(lmb.reserved.region[i].base,
			lmb_size_bytes(&lmb.reserved, i) - 1);
	}
}

void __init paging_init(void)
{
	int i;
	unsigned long zones_size[MAX_NR_ZONES];

	/* we can DMA to/from any address.  put all page into
	 * ZONE_DMA. */
	zones_size[ZONE_NORMAL] = max_low_pfn;

	/* every other zones are empty */
	for (i = 1; i < MAX_NR_ZONES; i++)
		zones_size[i] = 0;

	free_area_init_node(0, NODE_DATA(0), zones_size,
		NODE_DATA(0)->bdata->node_boot_start >> PAGE_SHIFT, NULL);
}

void free_init_pages(char *what, unsigned long begin, unsigned long end)
{
	unsigned long addr;

	for (addr = begin; addr < end; addr += PAGE_SIZE) {
		ClearPageReserved(virt_to_page(addr));
		init_page_count(virt_to_page(addr));
		memset((void *)addr, 0xcc, PAGE_SIZE);
		free_page(addr);
		totalram_pages++;
	}
	printk(KERN_INFO "Freeing %s: %ldk freed\n", what, (end - begin) >> 10);
}

#ifdef CONFIG_BLK_DEV_INITRD
void free_initrd_mem(unsigned long start, unsigned long end)
{
	int pages = 0;
	for (; start < end; start += PAGE_SIZE) {
		ClearPageReserved(virt_to_page(start));
		set_page_count(virt_to_page(start), 1);
		free_page(start);
		totalram_pages++;
		pages++;
	}
	printk(KERN_NOTICE "Freeing initrd memory: %dk freed\n", pages);
}
#endif

void free_initmem(void)
{
	free_init_pages("unused kernel memory",
			(unsigned long)(&__init_begin),
			(unsigned long)(&__init_end));
}

/* FIXME from arch/powerpc/mm/mem.c*/
void show_mem(void)
{
	printk(KERN_NOTICE "%s\n", __func__);
}

void __init mem_init(void)
{
	high_memory = (void *)(memory_end);

	/* this will put all memory onto the freelists */
	totalram_pages += free_all_bootmem();

	printk(KERN_INFO "Memory: %luk/%luk available\n",
	       (unsigned long) nr_free_pages() << (PAGE_SHIFT-10),
	       num_physpages << (PAGE_SHIFT-10));
}

/* Check against bounds of physical memory */
int ___range_ok(unsigned long addr, unsigned long size)
{
	return ((addr < memory_start) ||
		((addr + size) >= memory_end));
}
