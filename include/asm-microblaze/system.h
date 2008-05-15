/*
 * include/asm-microblaze/system.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 */

#ifndef _ASM_MICROBLAZE_SYSTEM_H
#define _ASM_MICROBLAZE_SYSTEM_H

#include <linux/autoconf.h>
#include <asm/registers.h>

struct task_struct;
struct thread_info;

extern struct task_struct *_switch_to(struct thread_info *prev,
					struct thread_info *next);

#define switch_to(prev, next, last)					\
	do {								\
		(last) = _switch_to(task_thread_info(prev),		\
					task_thread_info(next));	\
	} while (0)

#if CONFIG_XILINX_MICROBLAZE0_USE_MSR_INSTR

#define local_irq_save(flags)				\
	do {						\
		asm volatile ("# local_irq_save	\n\t"	\
				"msrclr %0, %1	\n\t"	\
				"nop	\n\t"		\
				: "=r"(flags)		\
				: "i"(MSR_IE)		\
				: "memory");		\
	} while (0)

#define local_irq_disable()					\
	do {							\
		asm volatile ("# local_irq_disable \n\t"	\
				"msrclr r0, %0 \n\t"		\
				"nop	\n\t"			\
				:				\
				: "i"(MSR_IE)			\
				: "memory");			\
	} while (0)

#define local_irq_enable()					\
	do {							\
		asm volatile ("# local_irq_enable \n\t"		\
				"msrset	r0, %0 \n\t"		\
				"nop	\n\t"			\
				:				\
				: "i"(MSR_IE)			\
				: "memory");			\
	} while (0)

#else /* CONFIG_XILINX_MICROBLAZE0_USE_MSR_INSTR == 0 */

#define local_irq_save(flags)					\
	do {							\
		register unsigned tmp;				\
		asm volatile ("# local_irq_save	\n\t"		\
				"mfs	%0, rmsr \n\t"		\
				"andi	%1, %0, %2 \n\t"	\
				"mts	rmsr, %1 \n\t"		\
				"nop \n\t"			\
				: "=r"(flags), "=r" (tmp)	\
				: "i"(~MSR_IE)			\
				: "memory");			\
	} while (0)

#define local_irq_disable()					\
	do {							\
		register unsigned tmp;				\
		asm volatile ("# local_irq_disable \n\t"	\
				"mfs	%0, rmsr \n\t"		\
				"andi	%0, %0, %1 \n\t"	\
				"mts	rmsr, %0 \n\t"		\
				"nop \n\t"			\
				: "=r"(tmp)			\
				: "i"(~MSR_IE)			\
				: "memory");			\
	} while (0)

#define local_irq_enable()					\
	do {							\
		register unsigned tmp;				\
		asm volatile ("# local_irq_enable \n\t"		\
				"mfs	%0, rmsr \n\t"		\
				"ori	%0, %0, %1 \n\t"	\
				"mts	rmsr, %0 \n\t"		\
				"nop \n\t"			\
				: "=r"(tmp)			\
				: "i"(MSR_IE)			\
				: "memory");			\
	} while (0)

#endif /* CONFIG_XILINX_MICROBLAZE0_USE_MSR_INSTR */

#define local_save_flags(flags)					\
	do {							\
		asm volatile ("# local_save_flags \n\t"		\
				"mfs	%0, rmsr \n\t"		\
				"nop	\n\t"			\
				: "=r"(flags)			\
				:				\
				: "memory");			\
	} while (0)

#define local_irq_restore(flags)			\
	do {						\
		asm volatile ("# local_irq_restore \n\t"\
				"mts	rmsr, %0 \n\t"	\
				"nop	\n\t"		\
				:			\
				:"r"(flags)		\
				: "memory");		\
	} while (0)

static inline int irqs_disabled(void)
{
	unsigned long flags;

	local_save_flags(flags);
	return ((flags & MSR_IE) == 0);
}

#define smp_read_barrier_depends()	do {} while (0)
#define read_barrier_depends()		do {} while (0)

#define nop()			asm volatile ("nop")
#define mb()			barrier()
#define rmb()			mb()
#define wmb()			mb()
#define set_mb(var, value)	do { var = value; mb(); } while (0)
#define set_wmb(var, value)	do { var = value; wmb(); } while (0)

#define smp_mb()		mb()
#define smp_rmb()		rmb()
#define smp_wmb()		wmb()

void show_trace(struct task_struct *task, unsigned long *stack);
void __bad_xchg(volatile void *ptr, int size);

static inline unsigned long __xchg(unsigned long x, volatile void *ptr,
								int size)
{
	unsigned long ret;
	unsigned long flags;

	switch (size) {
	case 1:
		local_irq_save(flags);
		ret = *(volatile unsigned char *)ptr;
		*(volatile unsigned char *)ptr = x;
		local_irq_restore(flags);
		break;

	case 4:
		local_irq_save(flags);
		ret = *(volatile unsigned long *)ptr;
		*(volatile unsigned long *)ptr = x;
		local_irq_restore(flags);
		break;
	default:
		__bad_xchg(ptr, size), ret = 0;
		break;
	}

	return ret;
}

#define xchg(ptr,x) \
	((__typeof__(*(ptr))) __xchg((unsigned long)(x), (ptr), sizeof(*(ptr))))

extern void *cacheable_memcpy(void *, const void *, unsigned int);
void free_init_pages(char *what, unsigned long begin, unsigned long end);
void free_initmem(void);
extern char *klimit;
extern void ret_from_fork(void);

#ifdef CONFIG_MTD_UCLINUX
extern char *_ebss;
#endif

#endif /* _ASM_MICROBLAZE_SYSTEM_H */
