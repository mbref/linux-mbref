/*
 * include/asm-microblaze/selfmod.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 Michal Simek <monstr@monstr.eu>
 */

#ifndef _ASM_MICROBLAZE_HACK_H
#define _ASM_MICROBLAZE_HACK_H

/*
 * HACK_BASE_ADDR is constant address for hack function.
 * do not change this value - it is hardcoded in hack function
 * arch/microblaze/kernel/hack.c:function_hack()
 */

#define HACK_BASE_ADDR	0x1234ff00

void function_hack(const int *arr_fce, const unsigned int base);

#endif /* _ASM_MICROBLAZE_HACK_H */
