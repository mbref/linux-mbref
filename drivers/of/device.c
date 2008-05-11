#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>

#include <asm/errno.h>

/**
 * of_match_device - Tell if an of_device structure has a matching
 * of_match structure
 * @ids: array of of device match structures to search in
 * @dev: the of device structure to match against
 *
 * Used by a driver to check whether an of_device present in the
 * system is in its list of supported devices.
 */
const struct of_device_id *of_match_device(const struct of_device_id *matches,
					const struct of_device *dev)
{
	if (!dev->node)
		return NULL;
	return of_match_node(matches, dev->node);
}
EXPORT_SYMBOL(of_match_device);

struct of_device *of_dev_get(struct of_device *dev)
{
	struct device *tmp;

	if (!dev)
		return NULL;
	tmp = get_device(&dev->dev);
	if (tmp)
		return to_of_device(tmp);
	else
		return NULL;
}
EXPORT_SYMBOL(of_dev_get);

void of_dev_put(struct of_device *dev)
{
	if (dev)
		put_device(&dev->dev);
}
EXPORT_SYMBOL(of_dev_put);

static ssize_t dev_show_devspec(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct of_device *ofdev;

	ofdev = to_of_device(dev);
	return sprintf(buf, "%s", ofdev->node->full_name);
}

static DEVICE_ATTR(devspec, S_IRUGO, dev_show_devspec, NULL);

/**
 * of_release_dev - free an of device structure when all users of it are finished.
 * @dev: device that's been disconnected
 *
 * Will be called only by the device core when all users of this of device are
 * done.
 */
void of_release_dev(struct device *dev)
{
	struct of_device *ofdev;

	ofdev = to_of_device(dev);
	of_node_put(ofdev->node);
	kfree(ofdev);
}
EXPORT_SYMBOL(of_release_dev);

int of_device_register(struct of_device *ofdev)
{
	int rc;

	BUG_ON(ofdev->node == NULL);

	rc = device_register(&ofdev->dev);
	if (rc)
		return rc;

	rc = device_create_file(&ofdev->dev, &dev_attr_devspec);
	if (rc)
		device_unregister(&ofdev->dev);

	return rc;
}
EXPORT_SYMBOL(of_device_register);

void of_device_unregister(struct of_device *ofdev)
{
	device_remove_file(&ofdev->dev, &dev_attr_devspec);
	device_unregister(&ofdev->dev);
}
EXPORT_SYMBOL(of_device_unregister);


struct of_device *of_device_alloc(struct device_node *np,
				  const char *bus_id,
				  struct device *parent)
{
	struct of_device *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	dev->node = of_node_get(np);
	dev->dev.dma_mask = &dev->dma_mask;
	dev->dev.parent = parent;
	dev->dev.release = of_release_dev;
	dev->dev.archdata.of_node = np;
#ifdef CONFIG_NUMA
	dev->dev.archdata.numa_node = of_node_to_nid(np);
#endif

	if (bus_id)
		strlcpy(dev->dev.bus_id, bus_id, BUS_ID_SIZE);
	else
		of_device_make_bus_id(dev);

	return dev;
}
EXPORT_SYMBOL(of_device_alloc);

ssize_t of_device_get_modalias(struct of_device *ofdev,
				char *str, ssize_t len)
{
	const char *compat;
	int cplen, i;
	ssize_t tsize, csize, repend;

	/* Name & Type */
	csize = snprintf(str, len, "of:N%sT%s",
				ofdev->node->name, ofdev->node->type);

	/* Get compatible property if any */
	compat = of_get_property(ofdev->node, "compatible", &cplen);
	if (!compat)
		return csize;

	/* Find true end (we tolerate multiple \0 at the end */
	for (i = (cplen-1); i >= 0 && !compat[i]; i--)
		cplen--;
	if (!cplen)
		return csize;
	cplen++;

	/* Check space (need cplen+1 chars including final \0) */
	tsize = csize + cplen;
	repend = tsize;

	/* @ the limit, all is already filled */
	if (csize >= len)
		return tsize;

	/* limit compat list */
	if (tsize >= len) {
		cplen = len - csize - 1;
		repend = len;
	}

	/* Copy and do char replacement */
	memcpy(&str[csize+1], compat, cplen);
	for (i = csize; i < repend; i++) {
		char c = str[i];
		if (c == '\0')
			str[i] = 'C';
		else if (c == ' ')
			str[i] = '_';
	}

	return tsize;
}
EXPORT_SYMBOL(of_device_get_modalias);

int of_device_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct of_device *ofdev;
	const char *compat;
	int seen = 0, cplen, sl;

	if (!dev)
		return -ENODEV;

	ofdev = to_of_device(dev);

	if (add_uevent_var(env, "OF_NAME=%s", ofdev->node->name))
		return -ENOMEM;

	if (add_uevent_var(env, "OF_TYPE=%s", ofdev->node->type))
		return -ENOMEM;

	/* Since the compatible field can contain pretty much anything
	 * it's not really legal to split it out with commas. We split it
	 * up using a number of environment variables instead. */

	compat = of_get_property(ofdev->node, "compatible", &cplen);
	while (compat && *compat && cplen > 0) {
		if (add_uevent_var(env, "OF_COMPATIBLE_%d=%s", seen, compat))
			return -ENOMEM;

		sl = strlen(compat) + 1;
		compat += sl;
		cplen -= sl;
		seen++;
	}

	if (add_uevent_var(env, "OF_COMPATIBLE_N=%d", seen))
		return -ENOMEM;

	/* modalias is trickier, we add it in 2 steps */
	if (add_uevent_var(env, "MODALIAS="))
		return -ENOMEM;
	sl = of_device_get_modalias(ofdev, &env->buf[env->buflen-1],
				    sizeof(env->buf) - env->buflen);
	if (sl >= (sizeof(env->buf) - env->buflen))
		return -ENOMEM;
	env->buflen += sl;

	return 0;
}
EXPORT_SYMBOL(of_device_uevent);
