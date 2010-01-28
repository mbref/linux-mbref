#ifndef _LINUX_UIO_PDRV_GENIRQ_H
#define _LINUX_UIO_PDRV_GENIRQ_H

struct uio_pdrv_genirq_platdata {
	struct uio_info *uioinfo;
	spinlock_t lock;
	unsigned long flags;
};

extern int __uio_pdrv_genirq_probe(struct device *dev, struct uio_info *uioinfo,
	struct resource *resources, unsigned int num_resources);

#endif
