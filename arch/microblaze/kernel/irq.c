/*
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/irqflags.h>
#include <linux/seq_file.h>
#include <linux/kernel_stat.h>
#include <linux/irq.h>

#include <asm/prom.h>

unsigned int irq_of_parse_and_map(struct device_node *dev, int index)
{
	struct of_irq oirq;

	if (of_irq_map_one(dev, index, &oirq))
		return NO_IRQ;

	return oirq.specifier[0];
}
EXPORT_SYMBOL_GPL(irq_of_parse_and_map);

/*
 * 'what should we do if we get a hw irq event on an illegal vector'.
 * each architecture has to answer this themselves.
 */
void ack_bad_irq(unsigned int irq)
{
	printk(KERN_WARNING "unexpected IRQ trap at vector %02x\n", irq);
}

void do_IRQ(struct pt_regs *regs)
{
	unsigned int irq;
	struct irq_desc *desc;

	irq_enter();
	set_irq_regs(regs);
	irq = get_irq(regs);
	BUG_ON(irq == -1U);
	desc = irq_desc + irq;
	desc->handle_irq(irq, desc);
	desc->chip->end(irq);
	irq_exit();
}

int show_interrupts(struct seq_file *p, void *v)
{
	int i = *(loff_t *) v, j;
	struct irqaction *action;
	unsigned long flags;

	if (i == 0) {
		seq_printf(p, "		");
		for_each_online_cpu(j)
			seq_printf(p, "CPU%-8d", j);
		seq_putc(p, '\n');
	}

	if (i < nr_irq) {
		spin_lock_irqsave(&irq_desc[i].lock, flags);
		action = irq_desc[i].action;
		if (!action)
			goto skip;
		seq_printf(p, "%3d: ", i);
#ifndef CONFIG_SMP
		seq_printf(p, "%10u ", kstat_irqs(i));
#else
		for_each_online_cpu(j)
			seq_printf(p, "%10u ", kstat_cpu(j).irqs[i]);
#endif
		seq_printf(p, " %8s", irq_desc[i].status &
					IRQ_LEVEL ? "level": "edge");
		seq_printf(p, " %8s", irq_desc[i].chip->name);
		seq_printf(p, "  %s", action->name);

		for (action = action->next; action; action = action->next)
			seq_printf(p, ", %s", action->name);

		seq_putc(p, '\n');
skip:
		spin_unlock_irqrestore(&irq_desc[i].lock, flags);
	}
	return 0;
}
