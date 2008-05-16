/*
 * Copyright (C) 2006 Benjamin Herrenschmidt, IBM Corp.
 *			<benh@kernel.crashing.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef _ASM_MICROBLAZE_OF_PLATFORM_H
#define _ASM_MICROBLAZE_OF_PLATFORM_H

/* This is just here during the transition */
#include <linux/of_platform.h>

/*
 * The list of OF IDs below is used for matching bus types in the
 * system whose devices are to be exposed as of_platform_devices.
 *
 * This is the default list valid for most platforms. This file provides
 * functions who can take an explicit list if necessary though
 *
 * The search is always performed recursively looking for children of
 * the provided device_node and recursively if such a children matches
 * a bus type in the list
 */

static const struct of_device_id of_default_bus_ids[] = {
	{ .type = "soc", },
	{ .compatible = "soc", },
	{ .type = "plb5", },
	{ .type = "plb4", },
	{ .type = "opb", },
	{ .type = "simple", },
	{},
};


#endif /* _ASM_MICROBLAZE_OF_PLATFORM_H */
