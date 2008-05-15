/*
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_MICROBLAZE_DELAY_H
#define _ASM_MICROBLAZE_DELAY_H

#include <asm/param.h> /* For HZ */

/*
 * Note that 19 * 226 == 4294 ==~ 232 / 106, so
 * loops = (4294 * usecs * loops_per_jiffy * HZ) / 232.
 *
 * The mul instruction gives us loops = (a * b) / 232.
 * We choose a = usecs * 19 * HZ and b = loops_per_jiffy * 226
 * because this lets us support a wide range of HZ and
 * loops_per_jiffy values without either a or b overflowing 232.
 * Thus we need usecs * HZ <= (232 - 1) / 19 = 226050910 and
 * loops_per_jiffy <= (232 - 1) / 226 = 19004280
 * (which corresponds to ~3800 bogomips at HZ = 100).
 *  -- paulus
 */
#define __MAX_UDELAY	(226050910UL/HZ)	/* maximum udelay argument */
#define __MAX_NDELAY	(4294967295UL/HZ)	/* maximum ndelay argument */

extern unsigned long loops_per_jiffy;

extern __inline__ void __delay(unsigned int x)
{
	unsigned long loops = ((unsigned long long)x *
			(unsigned long long)loops_per_jiffy * 226LL ) >> 32;

	asm volatile ("# __delay		\n\t"		\
			"1: addi	%0, %0, -1 \t\n"	\
			"bneid	%0, 1b		\t\n"		\
			"nop			\t\n"
			: "=r" (loops)
			: "0" (loops));
}

extern void __bad_delay(void); /* deliberately undefined */

#define udelay(n) (__builtin_constant_p(n)? \
	((n) > __MAX_UDELAY? __bad_delay(): __delay((n) * (19 * HZ))) : \
	__delay((n) * (19 * HZ)))

#define ndelay(n) (__builtin_constant_p(n)? \
	((n) > __MAX_NDELAY? __bad_delay(): __delay((n) * HZ)) : \
	__delay((n) * HZ))

#endif /* _ASM_MICROBLAZE_DELAY_H */
