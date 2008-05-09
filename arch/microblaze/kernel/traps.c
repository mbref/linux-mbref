/*
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <asm/exceptions.h>
#include <asm/system.h>

void trap_init(void)
{
	initialize_exception_handlers();
	__enable_hw_exceptions();
}

void __bad_xchg(volatile void *ptr, int size)
{
	printk(KERN_INFO "xchg: bad data size: pc 0x%p, ptr 0x%p, size %d\n",
		__builtin_return_address(0), ptr, size);
	BUG();
}
EXPORT_SYMBOL(__bad_xchg);

static int kstack_depth_to_print = 24;

void show_trace(struct task_struct *task, unsigned long *stack)
{
	unsigned long addr;

	if (!stack)
		stack = (unsigned long *)&stack;

	printk(KERN_INFO "Call Trace: ");
#ifdef CONFIG_KALLSYMS
	printk(KERN_INFO "\n");
#endif
	while (!kstack_end(stack)) {
		addr = *stack++;
		if (__kernel_text_address(addr)) {
			printk(KERN_INFO "[<%08lx>] ", addr);
			print_symbol("%s\n", addr);
		}
	}
	printk(KERN_INFO "\n");
}

void show_stack(struct task_struct *task, unsigned long *sp)
{
	unsigned long *stack;
	int i;

	if (sp == NULL) {
		if (task)
			sp = (unsigned long *) ((struct thread_info *)
						(task->stack))->cpu_context.r1;
		else
			sp = (unsigned long *)&sp;
	}

	stack = sp;

	printk(KERN_INFO "\nStack:\n  ");

	for (i = 0; i < kstack_depth_to_print; i++) {
		if (kstack_end(sp))
			break;
		if (i && ((i % 8) == 0))
			printk("\n  ");
		printk("%08lx ", *sp++);
	}
	printk("\n");
	show_trace(task, stack);
}

void dump_stack(void)
{
	show_stack(NULL, NULL);
}
EXPORT_SYMBOL(dump_stack);
