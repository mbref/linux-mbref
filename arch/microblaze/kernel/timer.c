/*
 * Copyright (C) 2007-2008 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/interrupt.h>
#include <linux/profile.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <asm/cpuinfo.h>
#include <asm/setup.h>

#include <asm/prom.h>
#include <asm/irq.h>

#ifdef CONFIG_SELFMOD_TIMER
#include <asm/selfmod.h>
#define TIMER_BASE	BARRIER_BASE_ADDR
#else
static unsigned int timer_baseaddr;
#define TIMER_BASE	timer_baseaddr
#endif

#define TCSR0	(0x00)
#define TLR0	(0x04)
#define TCR0	(0x08)
#define TCSR1	(0x10)
#define TLR1	(0x14)
#define TCR1	(0x18)

#define TCSR_MDT	(1<<0)
#define TCSR_UDT	(1<<1)
#define TCSR_GENT	(1<<2)
#define TCSR_CAPT	(1<<3)
#define TCSR_ARHT	(1<<4)
#define TCSR_LOAD	(1<<5)
#define TCSR_ENIT	(1<<6)
#define TCSR_ENT	(1<<7)
#define TCSR_TINT	(1<<8)
#define TCSR_PWMA	(1<<9)
#define TCSR_ENALL	(1<<10)

static void timer_ack(void)
{
	iowrite32(ioread32(TIMER_BASE + TCSR0), TIMER_BASE + TCSR0);
}

irqreturn_t timer_interrupt(int irq, void *dev_id)
{
#ifdef CONFIG_HEART_BEAT
	heartbeat();
#endif
	timer_ack();

	write_seqlock(&xtime_lock);

	do_timer(1);
	update_process_times(user_mode(get_irq_regs()));
	profile_tick(CPU_PROFILING);

	write_sequnlock(&xtime_lock);

	return IRQ_HANDLED;
}

static struct irqaction timer_irqaction = {
	.handler = timer_interrupt,
	.flags = IRQF_DISABLED,
	.name = "timer",
};

//void system_timer_init(void)
void time_init(void)
{
	int irq, j = 0;
	struct device_node *timer = NULL;
#ifdef CONFIG_SELFMOD_TIMER
	unsigned int timer_baseaddr = 0;
	int arr_func[] = {
				(int)&timer_ack,
				0
			};
#endif
	char *timer_list[] = {
				"xlnx,xps-timer-1.00.a",
				"xlnx,opb-timer-1.00.b",
				"xlnx,opb-timer-1.00.a",
				NULL
			};

	for (j = 0; timer_list[j] != NULL; j++) {
		timer = of_find_compatible_node(NULL, NULL, timer_list[j]);
		if (timer)
			break;
	}

	timer_baseaddr = *(int *) of_get_property(timer, "reg", NULL);
	timer_baseaddr = (unsigned long) ioremap(timer_baseaddr, PAGE_SIZE);
	irq = *(int *) of_get_property(timer, "interrupts", NULL);
#ifdef CONFIG_SELFMOD_TIMER
	selfmod_function((int *) arr_func, timer_baseaddr);
#endif
	printk(KERN_INFO "%s #0 at 0x%08x, irq=%d\n",
		timer_list[j], timer_baseaddr, irq);

	/* set the initial value to the load register */
	iowrite32(cpuinfo.cpu_clock_freq/HZ, timer_baseaddr + TLR0);

	/* load the initial value */
	iowrite32(TCSR_LOAD, timer_baseaddr + TCSR0);

	/* see timer data sheet for detail
	 * !ENALL - don't enable 'em all
	 * !PWMA - disable pwm
	 * TINT - clear interrupt status
	 * ENT- enable timer itself
	 * EINT - enable interrupt
	 * !LOAD - clear the bit to let go
	 * ARHT - auto reload
	 * !CAPT - no external trigger
	 * !GENT - no external signal
	 * UDT - set the timer as down counter
	 * !MDT0 - generate mode
	 *
	 */
	iowrite32(TCSR_TINT|TCSR_ENT|TCSR_ENIT|TCSR_ARHT|TCSR_UDT,
			timer_baseaddr + TCSR0);

	setup_irq(irq, &timer_irqaction);
}
