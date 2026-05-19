// SPDX-License-Identifier: GPL-2.0
/*
 * Example: SDIO function driver using nv_sdio framework
 *
 * Matches placeholder vendor/device 0x1234:0x5678 when an SDIO function
 * is present. Without a matching card the module loads but probe is not called.
 *
 * Sysfs (when bound): .../reg  — byte at SDIO register offset 0
 *
 * Load:   sudo insmod nv_sdio_demo.ko
 * Test:   bus/sdio/app/bin/nv_test_sdio
 */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>

#include "nv_sdio.h"

#define DRV_NAME		"nv_sdio_demo"
#define NV_SDIO_DEMO_CHIP	"nv_sdio_demo"
#define NV_SDIO_DEMO_REG	0x00
#define NV_SDIO_DEMO_BLKSZ	512

/* Placeholder IDs for documentation / QEMU SDIO gadgets */
#define NV_SDIO_DEMO_VENDOR	0x1234
#define NV_SDIO_DEMO_DEVICE	0x5678

struct demo_priv {
	u8 reg;
};

static struct nv_sdio_dev *demo_sdev(struct device *dev)
{
	return nv_sdio_dev_from_func(dev_to_sdio_func(dev));
}

static ssize_t reg_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct nv_sdio_dev *sdev = demo_sdev(dev);
	struct demo_priv *priv = sdev ? sdev->priv : NULL;
	u8 val;
	int ret;

	if (!priv)
		return -ENODEV;

	ret = nv_sdio_readb(sdev, NV_SDIO_DEMO_REG, &val);
	if (ret)
		return ret;

	priv->reg = val;
	return sysfs_emit(buf, "0x%02x\n", priv->reg);
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct nv_sdio_dev *sdev = demo_sdev(dev);
	struct demo_priv *priv = sdev ? sdev->priv : NULL;
	unsigned int val;
	int ret;

	if (!priv)
		return -ENODEV;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	if (val > 0xff)
		return -EINVAL;

	ret = nv_sdio_writeb(sdev, NV_SDIO_DEMO_REG, (u8)val);
	if (ret)
		return ret;

	priv->reg = (u8)val;
	return count;
}

static DEVICE_ATTR_RW(reg);

static struct attribute *demo_attrs[] = {
	&dev_attr_reg.attr,
	NULL,
};

static const struct attribute_group demo_attr_group = {
	.attrs = demo_attrs,
};

static int demo_probe(struct nv_sdio_dev *dev, const struct sdio_device_id *id)
{
	struct demo_priv *priv;
	int ret;

	ret = nv_sdio_enable_func(dev);
	if (ret)
		return ret;

	ret = nv_sdio_set_block_size(dev, NV_SDIO_DEMO_BLKSZ);
	if (ret) {
		nv_sdio_disable_func(dev);
		return ret;
	}

	priv = devm_kzalloc(nv_sdio_dev_device(dev), sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		nv_sdio_disable_func(dev);
		return -ENOMEM;
	}

	dev->priv = priv;

	ret = devm_device_add_group(nv_sdio_dev_device(dev), &demo_attr_group);
	if (ret) {
		nv_sdio_disable_func(dev);
		return ret;
	}

	ret = nv_sdio_writeb(dev, NV_SDIO_DEMO_REG, 0);
	if (ret < 0)
		dev_warn(nv_sdio_dev_device(dev),
			 "nv_sdio_demo: register init skipped (%d)\n", ret);

	dev_info(nv_sdio_dev_device(dev),
		 "nv_sdio_demo: ready func=%u block_size=%u (sysfs reg)\n",
		 dev->func_num, dev->block_size);
	return 0;
}

static void demo_remove(struct nv_sdio_dev *dev)
{
	nv_sdio_disable_func(dev);
	dev->priv = NULL;
}

static const struct nv_sdio_ops demo_ops = {
	.probe	= demo_probe,
	.remove	= demo_remove,
};

static struct nv_sdio_driver demo_driver;

static const struct sdio_device_id demo_ids[] = {
	{
		SDIO_DEVICE(NV_SDIO_DEMO_VENDOR, NV_SDIO_DEMO_DEVICE),
		.driver_data = (kernel_ulong_t)&demo_driver,
	},
	{ }
};
MODULE_DEVICE_TABLE(sdio, demo_ids);

static int __init demo_init(void)
{
	demo_driver.name = DRV_NAME;
	demo_driver.owner = THIS_MODULE;
	demo_driver.id_table = demo_ids;
	demo_driver.ops = &demo_ops;

	return nv_sdio_driver_register(&demo_driver);
}

static void __exit demo_exit(void)
{
	nv_sdio_driver_unregister(&demo_driver);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("SDIO demo driver (nv_sdio_demo)");
MODULE_LICENSE("GPL");
