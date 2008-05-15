/*
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_MICROBLAZE_ELF_H
#define _ASM_MICROBLAZE_ELF_H

#define EM_XILINX_MICROBLAZE 0xbaab
#define ELF_ARCH	EM_XILINX_MICROBLAZE

#define elf_check_arch(x) ((x)->e_machine == EM_XILINX_MICROBLAZE)

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS	ELFCLASS32

#endif /* _ASM_MICROBLAZE_ELF_H */
