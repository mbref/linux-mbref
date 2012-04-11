/*
 * `ptrace' system call
 *
 * Copyright (C) 2011 Billy Huang <billykhuang@gmail.com>
 * Copyright (C) 2008-2009 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2007-2009 PetaLogix
 * Copyright (C) 2004-2007 John Williams <john.williams@petalogix.com>
 *
 * derived from arch/v850/kernel/ptrace.c
 *
 * Copyright (C) 2002,03 NEC Electronics Corporation
 * Copyright (C) 2002,03 Miles Bader <miles@gnu.org>
 *
 * Derived from arch/mips/kernel/ptrace.c:
 *
 * Copyright (C) 1992 Ross Biro
 * Copyright (C) Linus Torvalds
 * Copyright (C) 1994, 95, 96, 97, 98, 2000 Ralf Baechle
 * Copyright (C) 1996 David S. Miller
 * Kevin D. Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999 MIPS Technologies, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file COPYING in the main directory of this
 * archive for more details.
 */

#include <asm/asm-offsets.h>
#include <asm/ptrace.h>
#include <linux/ptrace.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/elf.h>
#include <linux/audit.h>
#include <linux/seccomp.h>
#include <linux/tracehook.h>
#include <linux/user.h>
#include <linux/regset.h>

#include <linux/errno.h>
#include <asm/processor.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/syscall.h>
#include <asm/io.h>

#ifndef UTS_MACHINE
#define UTS_MACHINE "microblaze"
#endif

/* Returns the address where the register at REG_OFFS in P is stashed away. */
static microblaze_reg_t *reg_save_addr(unsigned reg_offs,
					struct task_struct *t)
{
	struct pt_regs *regs;

	/*
	 * Three basic cases:
	 *
	 * (1)	A register normally saved before calling the scheduler, is
	 *	available in the kernel entry pt_regs structure at the top
	 *	of the kernel stack. The kernel trap/irq exit path takes
	 *	care to save/restore almost all registers for ptrace'd
	 *	processes.
	 *
	 * (2)	A call-clobbered register, where the process P entered the
	 *	kernel via [syscall] trap, is not stored anywhere; that's
	 *	OK, because such registers are not expected to be preserved
	 *	when the trap returns anyway (so we don't actually bother to
	 *	test for this case).
	 *
	 * (3)	A few registers not used at all by the kernel, and so
	 *	normally never saved except by context-switches, are in the
	 *	context switch state.
	 */

	/* Register saved during kernel entry (or not available). */
	regs = task_pt_regs(t);

	return (microblaze_reg_t *)((char *)regs + reg_offs);
}

/* Get the general purpose registers */
static int gpr_get(struct task_struct *target, const struct user_regset *regset,
		unsigned int pos, unsigned int count,
		void *kbuf, void __user *ubuf)
{
	pr_debug("mb gpr_get: Get with count=%u\n", count);

	struct pt_regs *regs = task_pt_regs(target);
	int ret;

	/* Everything gets read out */
	ret = user_regset_copyout(&pos, &count, &kbuf, &ubuf,
				regs, 0, PT_MSR);

	return ret;
}

/* Set the general purpose registers */
static int gpr_set(struct task_struct *target, const struct user_regset *regset,
		unsigned int pos, unsigned int count,
		const void *kbuf, const void __user *ubuf)
{
	pr_debug("mb gpr_set: Set with count=%u\n", count);

	struct pt_regs *regs = task_pt_regs(target);
	int ret;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf,
				regs, 0, PT_MSR);

	return ret;

}

/*
 *  * These are our native regset flavors.
 *   */
enum microblaze_regset {
	REGSET_GPR,
};

static const struct user_regset native_regsets[] = {
	[REGSET_GPR] = {
		.core_note_type = NT_PRSTATUS,
		/* .n = 33 (0-31 gpr, 32 pc) */
		.n = ((PT_MSR)/sizeof(unsigned long)),
		/* all registers are 32-bits for the Microblaze */
		.size = sizeof(unsigned long),
		.align = sizeof(unsigned long),
		.get = gpr_get,
		.set = gpr_set
	},
};

static const struct user_regset_view user_microblaze_native_view = {
	.name = UTS_MACHINE,
	.e_machine = ELF_ARCH,
	.ei_osabi = ELF_OSABI,
	.regsets = native_regsets,
	.n = ARRAY_SIZE(native_regsets)
};

/*
 * NOTE:
 * Single stepping has been enabled in a basic form.
 *
 * FIXME???:
 * You'll notice I haven't entered any code in entry.S to check the syscfg flag
 * in kernel mode,  so perhaps some dangerous stuff could happen if trying to 
 * debug the kernel although I'm not completely sure - I haven't tested this.
 * All I know is that now GDB works natively on the microblaze, with breakpoints
 * and single stepping for a normal user program, and so have stopped coding
 * at this point.
 *
 * A general description for single stepping is this:
 *
 * GDB calls a breakpoint, initiating a SIGTRAP (breakpoint defined in
 * microblaze-tdep.h #define MICROBLAZE_BREAKPOINT {0xb9, 0xcc, 0x00, 0x60}),
 * This is handled in entry.S.
 *
 * Then what happens is a bit of a mystery:
 *
 * I think GDB executes the single command.
 * Then does ptrace (request=PTRACE_SINGLESTEP) is called.
 * This goes to kernel/ptrace.c which calls the arch/microblaze/kernel/ptrace.c,
 * (assuming you have added the appropriate #defines in arch/microblaze/include/asm/ptrace.h)
 * This sets TIF_SINGLESTEP (see user_enable_single_step below), and the pt_regs->syscfg
 * 
 * The the program being debugged (child) is resumed for an insruction.
 *
 * ~ Billy Huang.
 */

/* copied from arch/blackfin */
void user_enable_single_step(struct task_struct *child)
{
	struct pt_regs *regs = task_pt_regs(child);
	regs->syscfg |= SYSCFG_SSSTEP;  /* turn on bit */

	set_tsk_thread_flag(child, TIF_SINGLESTEP);
}

/* copied from arch/blackfin */
void user_disable_single_step(struct task_struct *child)
{
	struct pt_regs *regs = task_pt_regs(child);
	regs->syscfg &= ~SYSCFG_SSSTEP; /* turn off bit */

	clear_tsk_thread_flag(child, TIF_SINGLESTEP);
}

long arch_ptrace(struct task_struct *child, long request,
		 unsigned long addr, unsigned long data)
{
	int rval;

	unsigned long val = 0;

	pr_debug("mb arch_ptrace: Request Value: %ld\n", request);
	pr_debug("mb arch_ptrace: Address : 0x%08lx\n", addr);
	pr_debug("mb arch_ptrace: Data : 0x%08lx\n", data);

	switch (request) {
	/* Read/write the word at location ADDR in the registers. */
	case PTRACE_PEEKUSR:
	case PTRACE_POKEUSR:
		pr_debug("mb arch_ptrace: PEEKUSR/POKEUSR\n");
		rval = 0;
		if (addr >= PT_SIZE && request == PTRACE_PEEKUSR) {
			/*
			 * Special requests that don't actually correspond
			 * to offsets in struct pt_regs.
			 */
			if (addr == PT_TEXT_ADDR) {
				val = child->mm->start_code;
			} else if (addr == PT_DATA_ADDR) {
				val = child->mm->start_data;
			} else if (addr == PT_TEXT_LEN) {
				val = child->mm->end_code
					- child->mm->start_code;
			} else {
				rval = -EIO;
			}
		} else if (addr < PT_SIZE && (addr & 0x3) == 0) {
			microblaze_reg_t *reg_addr = reg_save_addr(addr, child);
			if (request == PTRACE_PEEKUSR)
				val = *reg_addr;
			else {
#if 1
				/* now we're poking... */
				*reg_addr = data;
#else
				/* MS potential problem on WB system
				 * Be aware that reg_addr is virtual address
				 * virt_to_phys conversion is necessary.
				 * This could be sensible solution.
				 */
				u32 paddr = virt_to_phys((u32)reg_addr);
				invalidate_icache_range(paddr, paddr + 4);
				*reg_addr = data;
				flush_dcache_range(paddr, paddr + 4);
#endif
			}
		} else
			rval = -EIO;

		if (rval == 0 && request == PTRACE_PEEKUSR)
			rval = put_user(val, (unsigned long __user *)data);
		break;
/* Doesn't work TO_REMOVE. These are standard ptrace defined routines,
 * so not defining here. See kernel/ptrace.c for more details on the
 * general routines. ~BH */
#if 0
	case PTRACE_PEEKDATA:
	case PTRACE_PEEKTEXT:	/* read word at location addr. */
	case PTRACE_POKEDATA:
	case PTRACE_POKETEXT:	/* write the word at location addr. */
#endif
	case PTRACE_GETREGS:
		pr_debug("mb arch_ptrace: PTRACE_GETREGS\n");
		rval = copy_regset_to_user(child, &user_microblaze_native_view,
					REGSET_GPR,
					0, PT_MSR,
					(void __user *) data);
		pr_debug("mb arch_ptrace: PTRACE_GETREGS rval %d\n", rval);
		break;
        case PTRACE_SETREGS:    /* Set all gp regs in the child. */
		pr_debug("mb arch_ptrace: PTRACE_SETREGS\n");
		rval = copy_regset_from_user(child, &user_microblaze_native_view,
					REGSET_GPR,
					0, PT_MSR,
					(const void __user *) data);
		pr_debug("mb arch_ptrace: PTRACE_SETREGS rval %d\n", rval);
		break;
        case PTRACE_SINGLESTEP:
		/* Enforce a continuation instead of single-step */
		/* request = PTRACE_CONT; // Removed as single-step now working! :-) */
		rval = ptrace_request(child, request, addr, data);
		break;
	default:
		rval = ptrace_request(child, request, addr, data);
		if (request == PTRACE_POKEDATA)
			pr_debug("mb arch_ptrace: PTRACE_POKEDATA (unsupported) rval %d\n", rval);
		if (request == PTRACE_CONT)
			pr_debug("mb arch_ptrace: PTRACE_CONT (unsupported) rval %d\n", rval);
		if (request == PTRACE_SINGLESTEP)
			pr_debug("mb arch_ptrace: PTRACE_SINGLESTEP (unsupported) rval %d\n", rval);
	}
	return rval;
}

asmlinkage long do_syscall_trace_enter(struct pt_regs *regs)
{
	long ret = 0;

	secure_computing_strict(regs->r12);

	if (test_thread_flag(TIF_SYSCALL_TRACE) &&
	    tracehook_report_syscall_entry(regs))
		/*
		 * Tracing decided this syscall should not happen.
		 * We'll return a bogus call number to get an ENOSYS
		 * error, but leave the original number in regs->regs[0].
		 */
		ret = -1L;

	audit_syscall_entry(EM_MICROBLAZE, regs->r12, regs->r5, regs->r6,
			    regs->r7, regs->r8);

	return ret ?: regs->r12;
}

asmlinkage void do_syscall_trace_leave(struct pt_regs *regs)
{
	int step;

	audit_syscall_exit(regs);

	step = test_thread_flag(TIF_SINGLESTEP);
	if (step || test_thread_flag(TIF_SYSCALL_TRACE))
		tracehook_report_syscall_exit(regs, step);
}

#if 0
static asmlinkage void syscall_trace(void)
{
	if (!test_thread_flag(TIF_SYSCALL_TRACE))
		return;
	if (!(current->ptrace & PT_PTRACED))
		return;
	/* The 0x80 provides a way for the tracing parent to distinguish
	 between a syscall stop and SIGTRAP delivery */
	ptrace_notify(SIGTRAP | ((current->ptrace & PT_TRACESYSGOOD)
				? 0x80 : 0));
	/*
	 * this isn't the same as continuing with a signal, but it will do
	 * for normal use. strace only continues with a signal if the
	 * stopping signal is not SIGTRAP. -brl
	 */
	if (current->exit_code) {
		send_sig(current->exit_code, current, 1);
		current->exit_code = 0;
	}
}
#endif

void ptrace_disable(struct task_struct *child)
{
	/* nothing to do */
}
