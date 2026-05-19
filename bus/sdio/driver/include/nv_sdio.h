/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - SDIO function helper layer
 */
#ifndef _NV_SDIO_H_
#define _NV_SDIO_H_

#include <linux/mmc/sdio_func.h>
#include <linux/module.h>
#include <linux/types.h>

struct nv_sdio_dev;
struct nv_sdio_driver;

/**
 * struct nv_sdio_ops - driver callbacks invoked by the framework
 */
struct nv_sdio_ops {
	int (*probe)(struct nv_sdio_dev *dev, const struct sdio_device_id *id);
	void (*remove)(struct nv_sdio_dev *dev);
};

/**
 * struct nv_sdio_dev - per-SDIO function context (sdio_set_drvdata)
 */
struct nv_sdio_dev {
	struct sdio_func *func;
	struct nv_sdio_driver *driver;
	const struct sdio_device_id *id;
	void *priv;

	unsigned int func_num;
	unsigned int class;
	unsigned short vendor;
	unsigned short device;
	unsigned int block_size;
};

/**
 * struct nv_sdio_driver - SDIO driver registration wrapper
 */
struct nv_sdio_driver {
	const char *name;
	struct module *owner;
	const struct sdio_device_id *id_table;
	const struct nv_sdio_ops *ops;

	struct sdio_driver sdio_driver;
	bool registered;
};

int nv_sdio_driver_register(struct nv_sdio_driver *drv);
void nv_sdio_driver_unregister(struct nv_sdio_driver *drv);

struct nv_sdio_dev *nv_sdio_dev_from_func(struct sdio_func *func);

int nv_sdio_enable_func(struct nv_sdio_dev *dev);
int nv_sdio_disable_func(struct nv_sdio_dev *dev);
int nv_sdio_set_block_size(struct nv_sdio_dev *dev, unsigned int blksz);

void nv_sdio_claim_host(struct nv_sdio_dev *dev);
void nv_sdio_release_host(struct nv_sdio_dev *dev);

int nv_sdio_readb(struct nv_sdio_dev *dev, unsigned int addr, u8 *val);
int nv_sdio_writeb(struct nv_sdio_dev *dev, unsigned int addr, u8 val);
int nv_sdio_readw(struct nv_sdio_dev *dev, unsigned int addr, u16 *val);
int nv_sdio_writew(struct nv_sdio_dev *dev, unsigned int addr, u16 val);
int nv_sdio_readl(struct nv_sdio_dev *dev, unsigned int addr, u32 *val);
int nv_sdio_writel(struct nv_sdio_dev *dev, unsigned int addr, u32 val);
int nv_sdio_memcpy_fromio(struct nv_sdio_dev *dev, void *dst,
			  unsigned int addr, int count);
int nv_sdio_memcpy_toio(struct nv_sdio_dev *dev, unsigned int addr,
			const void *src, int count);

int nv_sdio_claim_irq(struct nv_sdio_dev *dev, sdio_irq_handler_t *handler);
int nv_sdio_release_irq(struct nv_sdio_dev *dev);

static inline struct device *nv_sdio_dev_device(struct nv_sdio_dev *dev)
{
	return dev && dev->func ? &dev->func->dev : NULL;
}

#endif /* _NV_SDIO_H_ */
