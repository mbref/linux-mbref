/*
 * Author: Stephan Linz <linz@li-pro.net>
 *
 * Copyright (C) 2011 Li-Pro.Net, Stephan Linz <linz@li-pro.net>
 * Copyright (C) 2010 Sensor to image GmbH
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */
#ifndef _LINUX_S2I_H_
#define _LINUX_S2I_H_

/* some useful structs and typedefs for s2imac.c */
typedef struct {
	__u32	address;
	__u32	data;
} s2igen_content_t;

/* some useful defines for s2imac.c */
#define SIOS2IGENRD		SIOCDEVPRIVATE+0	/* generic read */
#define SIOS2IGENWR		SIOCDEVPRIVATE+1	/* generic write */
#define SIOS2IREGRD		SIOCDEVPRIVATE+2	/* register read */
#define SIOS2IREGWR		SIOCDEVPRIVATE+3	/* register write */

#endif /* _LINUX_S2I_H_ */
