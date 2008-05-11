#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>
#include <linux/of_device.h>

#include <asm/errno.h>

void of_device_make_bus_id(struct of_device *dev)
{
	static atomic_t bus_no_reg_magic;
	struct device_node *node = dev->node;
	char *name = dev->dev.bus_id;
	const u32 *reg;
	u64 addr;
	int magic;

	/*
	 * For MMIO, get the physical address
	 */
	reg = of_get_property(node, "reg", NULL);
	if (reg) {
		addr = of_translate_address(node, reg);
		if (addr != OF_BAD_ADDR) {
			snprintf(name, BUS_ID_SIZE,
				 "%llx.%s", (unsigned long long)addr,
				 node->name);
			return;
		}
	}

	/*
	 * No BusID, use the node name and add a globally incremented
	 * counter (and pray...)
	 */
	magic = atomic_add_return(1, &bus_no_reg_magic);
	snprintf(name, BUS_ID_SIZE, "%s.%d", node->name, magic - 1);
}
EXPORT_SYMBOL(of_device_make_bus_id);
