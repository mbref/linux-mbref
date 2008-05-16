/*
 * Copyright (C) 2007 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/autoconf.h>
#include <asm/page.h>
#include <asm/io.h>

#include <asm/prom.h>
#include <asm/irq.h>

#ifdef CONFIG_HACK
#include <asm/hack.h>
#else
static unsigned int intc_baseaddr;
#endif

unsigned int NR_IRQ;

/* No one else should require these constants, so define them locally here. */
#define ISR 0x00			/* Interrupt Status Register */
#define IPR 0x04			/* Interrupt Pending Register */
#define IER 0x08			/* Interrupt Enable Register */
#define IAR 0x0c			/* Interrupt Acknowledge Register */
#define SIE 0x10			/* Set Interrupt Enable bits */
#define CIE 0x14			/* Clear Interrupt Enable bits */
#define IVR 0x18			/* Interrupt Vector Register */
#define MER 0x1c			/* Master Enable Register */

#define MER_ME (1<<0)
#define MER_HIE (1<<1)

static void intc_enable(unsigned int irq)
{
	unsigned int mask = (0x00000001 << (irq & 31));
	pr_debug("enable: %d\n", irq);
#ifdef CONFIG_HACK
	iowrite32(mask, HACK_BASE_ADDR + SIE);
#else
	iowrite32(mask, intc_baseaddr + SIE);
#endif
}

static void intc_disable(unsigned int irq)
{
	unsigned long mask = (0x00000001 << (irq & 31));
	pr_debug("disable: %d\n", irq);
#ifdef CONFIG_HACK
	iowrite32(mask, HACK_BASE_ADDR + CIE);
#else
	iowrite32(mask, intc_baseaddr + CIE);
#endif
}

static void intc_disable_and_ack(unsigned int irq)
{
	unsigned long mask = (0x00000001 << (irq & 31));
	pr_debug("disable_and_ack: %d\n", irq);
#ifdef CONFIG_HACK
	iowrite32(mask, HACK_BASE_ADDR + CIE);
	/* ack edge triggered intr */
	if (!(irq_desc[irq].status & IRQ_LEVEL))
		iowrite32(mask, HACK_BASE_ADDR + IAR);
#else
	iowrite32(mask, intc_baseaddr + CIE);
	/* ack edge triggered intr */
	if (!(irq_desc[irq].status & IRQ_LEVEL))
		iowrite32(mask, intc_baseaddr + IAR);
#endif
}

static void intc_end(unsigned int irq)
{
	unsigned long mask = (0x00000001 << (irq & 31));

	pr_debug("end: %d\n", irq);
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS))) {
#ifdef CONFIG_HACK
		iowrite32(mask, HACK_BASE_ADDR + SIE);
		/* ack level sensitive intr */
		if (irq_desc[irq].status & IRQ_LEVEL)
			iowrite32(mask, HACK_BASE_ADDR + IAR);
#else
		iowrite32(mask, intc_baseaddr + SIE);
		/* ack level sensitive intr */
		if (irq_desc[irq].status & IRQ_LEVEL)
			iowrite32(mask, intc_baseaddr + IAR);
#endif
	}
}

static struct irq_chip intc_dev = {
	.name = "INTC",
	.enable = intc_enable,
	.disable = intc_disable,
	.mask_ack  = intc_disable_and_ack,
	.end = intc_end,
};

unsigned int get_irq(struct pt_regs *regs)
{
	int irq;

	/*
	 * NOTE: This function is the one that needs to be improved in
	 * order to handle multiple interrupt controllers. It currently
	 * is hardcoded to check for interrupts only on the first INTC.
	 */
#ifdef CONFIG_HACK
	irq = ioread32(HACK_BASE_ADDR + IVR);
#else
	irq = ioread32(intc_baseaddr + IVR);
#endif
	pr_debug("get_irq: %d\n", irq);

	return irq;
}

void __init init_IRQ(void)
{
	int i, j, intr_type;

	struct device_node *intc = NULL;
#ifdef CONFIG_HACK
	unsigned int intc_baseaddr = 0;
	static int arr_func[] = {
				(int)&get_irq,
				(int)&intc_enable,
				(int)&intc_disable,
				(int)&intc_disable_and_ack,
				(int)&intc_end,
				0
			};
#endif
	static char *intc_list[] = {
				"xlnx,xps-intc-1.00.a",
				"xlnx,opb-intc-1.00.c",
				"xlnx,opb-intc-1.00.b",
				"xlnx,opb-intc-1.00.a",
				NULL
			};

	for (j = 0; intc_list[j] != NULL; j++) {
		intc = of_find_compatible_node(NULL, NULL, intc_list[j]);
		if (intc)
			break;
	}

	intc_baseaddr = *(int *) of_get_property(intc, "reg", NULL);
	intc_baseaddr = (unsigned long) ioremap(intc_baseaddr, PAGE_SIZE);
	NR_IRQ = *(int *) of_get_property(intc, "xlnx,num-intr-inputs", NULL);
	intr_type = *(int *) of_get_property(intc, "xlnx,kind-of-edge", NULL);
#ifdef CONFIG_HACK
	function_hack((int *) arr_func, intc_baseaddr);
#endif
	printk(KERN_INFO "%s #0 at 0x%08x, num_irq=%d, edge=0x%x\n",
		intc_list[j], intc_baseaddr, NR_IRQ, intr_type);

	/*
	 * Disable all external interrupts until they are
	 * explicity requested.
	 */
	iowrite32(0, intc_baseaddr + IER);

	/* Acknowledge any pending interrupts just in case. */
	iowrite32(0xffffffff, intc_baseaddr + IAR);

	/* Turn on the Master Enable. */
	iowrite32(MER_HIE | MER_ME, intc_baseaddr + MER);

	for (i = 0; i < NR_IRQ; ++i) {
		if (intr_type & (0x00000001 << i)) {
		        set_irq_chip_and_handler_name(i, &intc_dev,
				handle_edge_irq, intc_dev.name);
			irq_desc[i].status &= ~IRQ_LEVEL;
		} else {
		        set_irq_chip_and_handler_name(i, &intc_dev,
				handle_level_irq, intc_dev.name);
			irq_desc[i].status |= IRQ_LEVEL;
		}
	}
}
