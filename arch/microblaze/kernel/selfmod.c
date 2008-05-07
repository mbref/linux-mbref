/*
 * arch/microblaze/kernel/hack.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 Michal Simek <monstr@monstr.eu>
 */

#include <linux/interrupt.h>
#include <asm/selfmod.h>

#undef DEBUG

/* NOTE
 * self-modified part of code for improvement of interrupt controller
 * save instruction in interrupt rutine
 */
void function_hack(const int *arr_fce, const unsigned int base)
{
	unsigned int flags = 0;
	unsigned int j, i;
	unsigned int *addr = NULL;

	local_irq_save(flags);
	__disable_icache();

	/* zero terminated array */
	for (j = 0; arr_fce[j] != 0; j++) {
		/* get start address of function */
		addr = (unsigned int *) arr_fce[j];
		pr_debug("%s: func(%d) at 0x%x\n",
					__func__, j, (unsigned int) addr);
		for (i = 0;; i++) {
			pr_debug("%s: instruction code at %d: 0x%x\n",
						__func__, i, addr[i]);
			if (addr[i] == 0xb0001234) {
				/* detecting of lwi or swi instruction */
				if ((addr[i+1] & 0xec00ff00) == 0xe800ff00) {
					pr_debug("%s: curr instr, "
						"(%d):0x%x, "
						"next(%d):0x%x\n",
						 __func__, i, addr[i], i+1,
								addr[i+1]);
					addr[i] = 0xb0000000 + (base >> 16);
					addr[i+1] = (addr[i+1] & 0xffff00ff) +
							(base & 0xffff);
					__invalidate_icache(addr[i]);
					__invalidate_icache(addr[i+1]);
					pr_debug("%s: hack instr, "
						"(%d):0x%x, "
						"next(%d):0x%x\n",
						 __func__, i, addr[i], i+1,
								addr[i+1]);
				} else /* detection addik for ack */
				if ((addr[i+1] & 0xfc00ff00) == 0x3000ff00) {
					pr_debug("%s: curr instr, "
						"(%d):0x%x, "
						"next(%d):0x%x\n",
						 __func__, i, addr[i], i+1,
								addr[i+1]);
					addr[i] = 0xb0000000 + (base >> 16);
					addr[i+1] = (addr[i+1] & 0xffff00ff) +
							(base & 0xffff);
					__invalidate_icache(addr[i]);
					__invalidate_icache(addr[i+1]);
					pr_debug("%s: hack instr, "
						"(%d):0x%x, "
						"next(%d):0x%x\n",
						 __func__, i, addr[i], i+1,
								addr[i+1]);
				}
			} else if (addr[i] == 0xb60f0008) {
				pr_debug("%s: end of array %d\n",
							__func__, i);
				break;
			}
		}
	}
	local_irq_restore(flags);
} /* end of self-modified code */
