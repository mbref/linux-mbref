/*
 * OF wrapper to make use of the uio_pdrv_genirq-driver.
 *
 * Copyright (C) 2009 Wolfram Sang, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uio_driver.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/uio_pdrv_genirq.h>

#define OF_DRIVER_VERSION "1"

static __devinit int uio_of_genirq_probe(struct of_device *op,
		const struct of_device_id *match)
{
	struct uio_info *uioinfo;
	struct resource resources[MAX_UIO_MAPS];
	int i, ret;

	uioinfo = kzalloc(sizeof(*uioinfo), GFP_KERNEL);
	if (!uioinfo)
		return -ENOMEM;

	uioinfo->name = op->node->name;
	uioinfo->version = OF_DRIVER_VERSION;
	uioinfo->irq = irq_of_parse_and_map(op->node, 0);
	if (!uioinfo->irq)
		uioinfo->irq = UIO_IRQ_NONE;

	for (i = 0; i < MAX_UIO_MAPS; ++i)
		if (of_address_to_resource(op->node, i, &resources[i]))
			break;

	ret = __uio_pdrv_genirq_probe(&op->dev, uioinfo, resources, i);
	if (ret)
		goto err_cleanup;

	return 0;

err_cleanup:
	if (uioinfo->irq != UIO_IRQ_NONE)
		irq_dispose_mapping(uioinfo->irq);

	kfree(uioinfo);
	return ret;
}

static __devexit int uio_of_genirq_remove(struct of_device *op)
{
	struct uio_pdrv_genirq_platdata *priv = dev_get_drvdata(&op->dev);

	uio_unregister_device(priv->uioinfo);

	if (priv->uioinfo->irq != UIO_IRQ_NONE)
		irq_dispose_mapping(priv->uioinfo->irq);

	kfree(priv->uioinfo);
	kfree(priv);
	return 0;
}

/* Match table for of_platform binding */
static const struct of_device_id __devinitconst uio_of_genirq_match[] = {
	{ .compatible = "generic-uio", },
	{},
};
MODULE_DEVICE_TABLE(of, uio_of_genirq_match);

static struct of_platform_driver uio_of_genirq_driver = {
	.owner = THIS_MODULE,
	.name = "uio-of-genirq",
	.match_table = uio_of_genirq_match,
	.probe = uio_of_genirq_probe,
	.remove = __devexit_p(uio_of_genirq_remove),
};

static inline int __init uio_of_genirq_init(void)
{
	return of_register_platform_driver(&uio_of_genirq_driver);
}
module_init(uio_of_genirq_init);

static inline void __exit uio_of_genirq_exit(void)
{
	of_unregister_platform_driver(&uio_of_genirq_driver);
}
module_exit(uio_of_genirq_exit);

MODULE_AUTHOR("Wolfram Sang");
MODULE_DESCRIPTION("Userspace I/O OF driver with generic IRQ handling");
MODULE_LICENSE("GPL v2");
