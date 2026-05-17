/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - block device helper layer
 */
#ifndef _NV_BLOCK_H_
#define _NV_BLOCK_H_

#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/types.h>

struct nv_block_dev;
struct nv_block_driver;

/**
 * struct nv_block_ops - per-disk block operation callbacks
 *
 * submit_bio is required for I/O. open/release/ioctl are optional.
 */
struct nv_block_ops {
	blk_status_t (*submit_bio)(struct nv_block_dev *dev, struct bio *bio);
	int (*open)(struct nv_block_dev *dev, struct gendisk *disk,
		    blk_mode_t mode);
	void (*release)(struct nv_block_dev *dev, struct gendisk *disk);
	int (*ioctl)(struct nv_block_dev *dev, struct block_device *bdev,
		     blk_mode_t mode, unsigned int cmd, unsigned long arg);
};

/**
 * struct nv_block_dev - one block device (gendisk) instance
 */
struct nv_block_dev {
	struct nv_block_driver *driver;
	unsigned int index;
	const char *name;
	const struct nv_block_ops *ops;
	void *priv;

	sector_t capacity;
	unsigned int logical_block_size;
	struct gendisk *disk;
	bool registered;
};

/**
 * struct nv_block_driver - driver-wide bookkeeping
 */
struct nv_block_driver {
	const char *name;
	struct module *owner;
	unsigned int count;

	struct nv_block_dev **devices;
};

int nv_block_driver_register(struct nv_block_driver *drv);
void nv_block_driver_unregister(struct nv_block_driver *drv);

int nv_block_device_register(struct nv_block_driver *drv,
			      struct nv_block_dev *dev, unsigned int index,
			      const char *name,
			      const struct nv_block_ops *ops, void *priv,
			      sector_t capacity_sectors,
			      unsigned int logical_block_size);
void nv_block_device_unregister(struct nv_block_dev *dev);

#endif /* _NV_BLOCK_H_ */
