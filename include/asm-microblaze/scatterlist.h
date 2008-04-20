/*
 * include/asm-microblaze/scatterlist.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 */

#ifndef _ASM_MICROBLAZE_SCATTERLIST_H
#define _ASM_MICROBLAZE_SCATTERLIST_H

struct scatterlist {
	unsigned long page_link;
	unsigned int	offset;
	dma_addr_t	dma_address;
	unsigned int	length;
};

#endif /* _ASM_MICROBLAZE_SCATTERLIST_H */
