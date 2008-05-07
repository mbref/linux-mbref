/*
 * include/asm-microblaze/irq.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 */

#ifndef _ASM_MICROBLAZE_IRQ_H
#define _ASM_MICROBLAZE_IRQ_H

#include <linux/seq_file.h>
#define NR_IRQS 32
#include <linux/irq.h>

extern unsigned int NR_IRQ;
extern void ledoff(void);

#define NO_IRQ (-1)

static inline int irq_canonicalize(int irq)
{
	return (irq);
}

struct pt_regs;
extern void do_IRQ(struct pt_regs *regs);
extern void __init init_IRQ(void);
int show_interrupts(struct seq_file *p, void *v);
irqreturn_t timer_interrupt(int irq, void *dev_id);

/* irq_of_parse_and_map - Parse and Map an interrupt into linux virq space
 * @device: Device node of the device whose interrupt is to be mapped
 * @index: Index of the interrupt to map
 *
 * This function is a wrapper that chains of_irq_map_one() and
 * irq_create_of_mapping() to make things easier to callers
 */
struct device_node;
extern unsigned int irq_of_parse_and_map(struct device_node *dev, int index);

#endif /* _ASM_MICROBLAZE_IRQ_H */
