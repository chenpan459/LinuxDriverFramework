// SPDX-License-Identifier: GPL-2.0
/*
 * Example: I2C driver using nv_i2c framework
 *
 * Instantiate (after insmod) on any bus, e.g. i2c-stub:
 *   echo nv_i2c_demo 0x50 > /sys/bus/i2c/devices/i2c-0/new_device
 *
 * Sysfs: .../reg  (byte register for app test)
 */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>

#include "nv_i2c.h"

#define DRV_NAME		"nv_i2c_demo"
#define NV_I2C_DEMO_CHIP	"nv_i2c_demo"
#define NV_I2C_DEMO_REG		0x00

struct demo_priv {
	u8 reg;
};

static struct nv_i2c_dev *demo_idev(struct device *dev)
{
	return nv_i2c_dev_from_client(to_i2c_client(dev));
}

static ssize_t reg_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct nv_i2c_dev *idev = demo_idev(dev);
	struct demo_priv *priv = idev ? idev->priv : NULL;

	if (!priv)
		return -ENODEV;

	return sysfs_emit(buf, "0x%02x\n", priv->reg);
}

static ssize_t reg_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct nv_i2c_dev *idev = demo_idev(dev);
	struct demo_priv *priv = idev ? idev->priv : NULL;
	unsigned int val;
	int ret;

	if (!priv)
		return -ENODEV;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	if (val > 0xff)
		return -EINVAL;

	priv->reg = (u8)val;

	ret = nv_i2c_smbus_write_byte_data(idev, NV_I2C_DEMO_REG, priv->reg);
	if (ret < 0)
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

static int demo_probe(struct nv_i2c_dev *dev, const struct i2c_device_id *id)
{
	struct demo_priv *priv;
	int ret;

	priv = devm_kzalloc(nv_i2c_dev_device(dev), sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	dev->priv = priv;

	ret = devm_device_add_group(nv_i2c_dev_device(dev), &demo_attr_group);
	if (ret)
		return ret;

	ret = nv_i2c_smbus_write_byte_data(dev, NV_I2C_DEMO_REG, 0);
	if (ret < 0)
		dev_warn(nv_i2c_dev_device(dev),
			 "nv_i2c_demo: smbus write skipped (%d)\n", ret);

	dev_info(nv_i2c_dev_device(dev),
		 "nv_i2c_demo: ready adapter=%u addr=0x%02x (sysfs reg)\n",
		 dev->adapter_nr, dev->addr);
	return 0;
}

static void demo_remove(struct nv_i2c_dev *dev)
{
	dev->priv = NULL;
}

static const struct nv_i2c_ops demo_ops = {
	.probe	= demo_probe,
	.remove	= demo_remove,
};

static struct nv_i2c_driver demo_driver;

static const struct i2c_device_id demo_ids[] = {
	{ NV_I2C_DEMO_CHIP, (kernel_ulong_t)&demo_driver },
	{ }
};
MODULE_DEVICE_TABLE(i2c, demo_ids);

static const struct of_device_id demo_of_match[] = {
	{ .compatible = "nv,i2c-demo", },
	{ }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static int __init demo_init(void)
{
	demo_driver.name = DRV_NAME;
	demo_driver.owner = THIS_MODULE;
	demo_driver.id_table = demo_ids;
	demo_driver.ops = &demo_ops;

	return nv_i2c_driver_register(&demo_driver);
}

static void __exit demo_exit(void)
{
	nv_i2c_driver_unregister(&demo_driver);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("I2C demo driver (nv_i2c_demo chip)");
MODULE_LICENSE("GPL");
