/*
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_MICROBLAZE_SECTIONS_H
#define _ASM_MICROBLAZE_SECTIONS_H

#include <asm-generic/sections.h>

extern char _ssbss[], _esbss[];
extern unsigned long __ivt_start[], __ivt_end[];

#ifdef CONFIG_MTD_UCLINUX
extern char *_ebss;
#endif

#endif /* _ASM_MICROBLAZE_SECTIONS_H */
