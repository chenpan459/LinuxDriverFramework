// SPDX-License-Identifier: GPL-2.0
/*
 * Example: SPI driver using nv_spi framework
 *
 * Device tree (optional):
 *   nv_spi_demo@0 {
 *       compatible = "nv,spi-demo";
 *       reg = <0>;
 *       spi-max-frequency = <1000000>;
 *   };
 *
 * Sysfs: .../reg  (byte value for app test)
 */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>

#include "nv_spi.h"

#define DRV_NAME		"nv_spi_demo"
#define NV_SPI_DEMO_CHIP	"nv_spi_demo"

struct demo_priv {
	u8 reg;
};

static struct nv_spi_dev *demo_sdev(struct device *dev)
{
	return nv_spi_dev_from_spi(to_spi_device(dev));
}

static ssize_t reg_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct nv_spi_dev *sdev = demo_sdev(dev);
	struct demo_priv *priv = sdev ? sdev->priv : NULL;

	if (!priv)
		return -ENODEV;

	return sysfs_emit(buf, "0x%02x\n", priv->reg);
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct nv_spi_dev *sdev = demo_sdev(dev);
	struct demo_priv *priv = sdev ? sdev->priv : NULL;
	unsigned int val;
	u8 tx[2];
	int ret;

	if (!priv)
		return -ENODEV;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	if (val > 0xff)
		return -EINVAL;

	priv->reg = (u8)val;

	/* cmd + data write (demo protocol) */
	tx[0] = 0x00;
	tx[1] = priv->reg;
	ret = nv_spi_write(sdev, tx, sizeof(tx));
	if (ret)
		return ret;

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

static int demo_probe(struct nv_spi_dev *dev, const struct spi_device_id *id)
{
	struct demo_priv *priv;
	u8 tx[2] = { 0x00, 0x00 };
	int ret;

	priv = devm_kzalloc(nv_spi_dev_device(dev), sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	dev->priv = priv;

	ret = devm_device_add_group(nv_spi_dev_device(dev), &demo_attr_group);
	if (ret)
		return ret;

	ret = nv_spi_write(dev, tx, sizeof(tx));
	if (ret)
		dev_warn(nv_spi_dev_device(dev),
			 "nv_spi_demo: initial write skipped (%d)\n", ret);

	dev_info(nv_spi_dev_device(dev),
		 "nv_spi_demo: ready spi%u.%u (sysfs reg)\n",
		 dev->bus_num, dev->chip_select);
	return 0;
}

static void demo_remove(struct nv_spi_dev *dev)
{
	dev->priv = NULL;
}

static const struct nv_spi_ops demo_ops = {
	.probe	= demo_probe,
	.remove	= demo_remove,
};

static struct nv_spi_driver demo_driver;

static const struct spi_device_id demo_ids[] = {
	{ NV_SPI_DEMO_CHIP, (kernel_ulong_t)&demo_driver },
	{ }
};
MODULE_DEVICE_TABLE(spi, demo_ids);

static const struct of_device_id demo_of_match[] = {
	{ .compatible = "nv,spi-demo", },
	{ }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static int __init demo_init(void)
{
	demo_driver.name = DRV_NAME;
	demo_driver.owner = THIS_MODULE;
	demo_driver.id_table = demo_ids;
	demo_driver.of_match_table = demo_of_match;
	demo_driver.ops = &demo_ops;

	return nv_spi_driver_register(&demo_driver);
}

static void __exit demo_exit(void)
{
	nv_spi_driver_unregister(&demo_driver);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("SPI demo driver (nv_spi_demo)");
MODULE_LICENSE("GPL");
