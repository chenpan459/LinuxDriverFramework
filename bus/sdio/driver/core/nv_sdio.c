// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - SDIO function helper implementation
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mmc/sdio_func.h>

#include "nv_sdio.h"

static struct nv_sdio_driver *nv_sdio_drv_from_func(struct sdio_func *func)
{
	struct sdio_driver *sdrv;

	if (!func || !func->dev.driver)
		return NULL;

	sdrv = container_of(func->dev.driver, struct sdio_driver, drv);
	return container_of(sdrv, struct nv_sdio_driver, sdio_driver);
}

static struct nv_sdio_driver *nv_sdio_drv_from_id(const struct sdio_device_id *id)
{
	if (!id || !id->driver_data)
		return NULL;

	return (struct nv_sdio_driver *)(unsigned long)id->driver_data;
}

struct nv_sdio_dev *nv_sdio_dev_from_func(struct sdio_func *func)
{
	return func ? sdio_get_drvdata(func) : NULL;
}
EXPORT_SYMBOL_GPL(nv_sdio_dev_from_func);

void nv_sdio_claim_host(struct nv_sdio_dev *dev)
{
	if (dev && dev->func)
		sdio_claim_host(dev->func);
}
EXPORT_SYMBOL_GPL(nv_sdio_claim_host);

void nv_sdio_release_host(struct nv_sdio_dev *dev)
{
	if (dev && dev->func)
		sdio_release_host(dev->func);
}
EXPORT_SYMBOL_GPL(nv_sdio_release_host);

int nv_sdio_enable_func(struct nv_sdio_dev *dev)
{
	if (!dev || !dev->func)
		return -EINVAL;

	return sdio_enable_func(dev->func);
}
EXPORT_SYMBOL_GPL(nv_sdio_enable_func);

int nv_sdio_disable_func(struct nv_sdio_dev *dev)
{
	if (!dev || !dev->func)
		return -EINVAL;

	return sdio_disable_func(dev->func);
}
EXPORT_SYMBOL_GPL(nv_sdio_disable_func);

int nv_sdio_set_block_size(struct nv_sdio_dev *dev, unsigned int blksz)
{
	int ret;

	if (!dev || !dev->func || !blksz)
		return -EINVAL;

	ret = sdio_set_block_size(dev->func, blksz);
	if (!ret)
		dev->block_size = blksz;
	return ret;
}
EXPORT_SYMBOL_GPL(nv_sdio_set_block_size);

int nv_sdio_readb(struct nv_sdio_dev *dev, unsigned int addr, u8 *val)
{
	int err = 0;
	u8 v;

	if (!dev || !dev->func || !val)
		return -EINVAL;

	v = sdio_readb(dev->func, addr, &err);
	if (err)
		return err;

	*val = v;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_sdio_readb);

int nv_sdio_writeb(struct nv_sdio_dev *dev, unsigned int addr, u8 val)
{
	int err = 0;

	if (!dev || !dev->func)
		return -EINVAL;

	sdio_writeb(dev->func, val, addr, &err);
	return err;
}
EXPORT_SYMBOL_GPL(nv_sdio_writeb);

int nv_sdio_readw(struct nv_sdio_dev *dev, unsigned int addr, u16 *val)
{
	int err = 0;
	u16 v;

	if (!dev || !dev->func || !val)
		return -EINVAL;

	v = sdio_readw(dev->func, addr, &err);
	if (err)
		return err;

	*val = v;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_sdio_readw);

int nv_sdio_writew(struct nv_sdio_dev *dev, unsigned int addr, u16 val)
{
	int err = 0;

	if (!dev || !dev->func)
		return -EINVAL;

	sdio_writew(dev->func, val, addr, &err);
	return err;
}
EXPORT_SYMBOL_GPL(nv_sdio_writew);

int nv_sdio_readl(struct nv_sdio_dev *dev, unsigned int addr, u32 *val)
{
	int err = 0;
	u32 v;

	if (!dev || !dev->func || !val)
		return -EINVAL;

	v = sdio_readl(dev->func, addr, &err);
	if (err)
		return err;

	*val = v;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_sdio_readl);

int nv_sdio_writel(struct nv_sdio_dev *dev, unsigned int addr, u32 val)
{
	int err = 0;

	if (!dev || !dev->func)
		return -EINVAL;

	sdio_writel(dev->func, val, addr, &err);
	return err;
}
EXPORT_SYMBOL_GPL(nv_sdio_writel);

int nv_sdio_memcpy_fromio(struct nv_sdio_dev *dev, void *dst,
			  unsigned int addr, int count)
{
	if (!dev || !dev->func || !dst || count <= 0)
		return -EINVAL;

	return sdio_memcpy_fromio(dev->func, dst, addr, count);
}
EXPORT_SYMBOL_GPL(nv_sdio_memcpy_fromio);

int nv_sdio_memcpy_toio(struct nv_sdio_dev *dev, unsigned int addr,
			const void *src, int count)
{
	if (!dev || !dev->func || !src || count <= 0)
		return -EINVAL;

	return sdio_memcpy_toio(dev->func, addr, (void *)src, count);
}
EXPORT_SYMBOL_GPL(nv_sdio_memcpy_toio);

int nv_sdio_claim_irq(struct nv_sdio_dev *dev, sdio_irq_handler_t *handler)
{
	if (!dev || !dev->func || !handler)
		return -EINVAL;

	return sdio_claim_irq(dev->func, handler);
}
EXPORT_SYMBOL_GPL(nv_sdio_claim_irq);

int nv_sdio_release_irq(struct nv_sdio_dev *dev)
{
	if (!dev || !dev->func)
		return -EINVAL;

	return sdio_release_irq(dev->func);
}
EXPORT_SYMBOL_GPL(nv_sdio_release_irq);

static int nv_sdio_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
	struct nv_sdio_driver *drv;
	struct nv_sdio_dev *dev;
	int ret;

	if (!func)
		return -EINVAL;

	drv = nv_sdio_drv_from_id(id);
	if (!drv)
		drv = nv_sdio_drv_from_func(func);
	if (!drv || !drv->ops || !drv->ops->probe)
		return -ENODEV;

	dev = devm_kzalloc(&func->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->func = func;
	dev->driver = drv;
	dev->id = id;
	dev->func_num = func->num;
	dev->class = func->class;
	dev->vendor = func->vendor;
	dev->device = func->device;
	dev->block_size = func->cur_blksize;
	sdio_set_drvdata(func, dev);

	ret = drv->ops->probe(dev, id);
	if (ret) {
		sdio_set_drvdata(func, NULL);
		return ret;
	}

	dev_info(&func->dev,
		 "nv_sdio: %s bound (func %u vendor 0x%04x device 0x%04x class 0x%02x)\n",
		 drv->name, dev->func_num, dev->vendor, dev->device, dev->class);
	return 0;
}

static void nv_sdio_remove(struct sdio_func *func)
{
	struct nv_sdio_driver *drv = nv_sdio_drv_from_func(func);
	struct nv_sdio_dev *dev = nv_sdio_dev_from_func(func);

	if (!dev)
		return;

	if (drv && drv->ops && drv->ops->remove)
		drv->ops->remove(dev);

	sdio_set_drvdata(func, NULL);
}

int nv_sdio_driver_register(struct nv_sdio_driver *drv)
{
	int ret;

	if (!drv || !drv->name || !drv->owner || !drv->id_table || !drv->ops)
		return -EINVAL;

	if (drv->registered)
		return -EBUSY;

	drv->sdio_driver.name = (char *)drv->name;
	drv->sdio_driver.id_table = drv->id_table;
	drv->sdio_driver.probe = nv_sdio_probe;
	drv->sdio_driver.remove = nv_sdio_remove;

	ret = __sdio_register_driver(&drv->sdio_driver, drv->owner);
	if (ret)
		return ret;

	drv->registered = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_sdio_driver_register);

void nv_sdio_driver_unregister(struct nv_sdio_driver *drv)
{
	if (!drv || !drv->registered)
		return;

	sdio_unregister_driver(&drv->sdio_driver);
	drv->registered = false;
}
EXPORT_SYMBOL_GPL(nv_sdio_driver_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("SDIO function framework");
MODULE_LICENSE("GPL");
