/*
 * Arch specific extensions to struct device
 *
 * This file is released under the GPLv2
 */

#ifndef _ASM_MICROBLAZE_DEVICE_H
#define _ASM_MICROBLAZE_DEVICE_H

struct device_node;

struct dev_archdata {
	/* Optional pointer to an OF device node */
	struct device_node	*of_node;
};

#endif /* _ASM_MICROBLAZE_DEVICE_H */


