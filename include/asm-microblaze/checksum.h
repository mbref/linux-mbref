/*
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_MICROBLAZE_CHECKSUM_H
#define _ASM_MICROBLAZE_CHECKSUM_H

/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented
 */
static inline unsigned int
csum_tcpudp_nofold(unsigned long saddr, unsigned long daddr, unsigned short len,
		unsigned short proto, unsigned int sum)
{
	__asm__("add %0, %4, %1\n\t"
		"addc %0, %4, %2\n\t"
		"addc %0, %4, %3\n\t"
		"addc %0, %4, r0\n\t"
		: "=d" (sum)
		: "d" (saddr), "d" (daddr), "d" (len + proto),
		"0"(sum));

	return sum;
}


#include <asm-generic/checksum.h>

#endif /* _ASM_MICROBLAZE_CHECKSUM_H */
