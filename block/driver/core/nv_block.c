// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - block device helper implementation
 */

#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "nv_block.h"

#ifndef BLK_DEF_MAX_SECTORS_CAP
#define BLK_DEF_MAX_SECTORS_CAP BLK_SAFE_MAX_SECTORS
#endif

static struct nv_block_dev *nv_block_dev_from_disk(struct gendisk *disk)
{
	return disk->private_data;
}

static void nv_block_submit_bio(struct bio *bio)
{
	struct nv_block_dev *dev;
	blk_status_t st = BLK_STS_IOERR;

	if (!bio || !bio->bi_bdev || !bio->bi_bdev->bd_disk)
		goto out;

	dev = nv_block_dev_from_disk(bio->bi_bdev->bd_disk);
	if (dev->ops && dev->ops->submit_bio)
		st = dev->ops->submit_bio(dev, bio);
out:
	bio->bi_status = st;
	bio_endio(bio);
}

static int nv_block_open(struct gendisk *disk, blk_mode_t mode)
{
	struct nv_block_dev *dev = nv_block_dev_from_disk(disk);

	if (dev->ops && dev->ops->open)
		return dev->ops->open(dev, disk, mode);

	return 0;
}

static void nv_block_release(struct gendisk *disk)
{
	struct nv_block_dev *dev = nv_block_dev_from_disk(disk);

	if (dev->ops && dev->ops->release)
		dev->ops->release(dev, disk);
}

static int nv_block_ioctl(struct block_device *bdev, blk_mode_t mode,
			   unsigned int cmd, unsigned long arg)
{
	struct nv_block_dev *dev = nv_block_dev_from_disk(bdev->bd_disk);

	if (dev->ops && dev->ops->ioctl)
		return dev->ops->ioctl(dev, bdev, mode, cmd, arg);

	return -ENOTTY;
}

static const struct block_device_operations nv_block_bdops = {
	.submit_bio	= nv_block_submit_bio,
	.open		= nv_block_open,
	.release	= nv_block_release,
	.ioctl		= nv_block_ioctl,
};

int nv_block_driver_register(struct nv_block_driver *drv)
{
	if (!drv || !drv->name || !drv->count || !drv->owner)
		return -EINVAL;

	drv->devices = kcalloc(drv->count, sizeof(*drv->devices), GFP_KERNEL);
	if (!drv->devices)
		return -ENOMEM;

	return 0;
}
EXPORT_SYMBOL_GPL(nv_block_driver_register);

void nv_block_driver_unregister(struct nv_block_driver *drv)
{
	unsigned int i;

	if (!drv)
		return;

	if (drv->devices) {
		for (i = 0; i < drv->count; i++) {
			if (drv->devices[i])
				nv_block_device_unregister(drv->devices[i]);
		}
		kfree(drv->devices);
		drv->devices = NULL;
	}
}
EXPORT_SYMBOL_GPL(nv_block_driver_unregister);

int nv_block_device_register(struct nv_block_driver *drv,
			      struct nv_block_dev *dev, unsigned int index,
			      const char *name, const struct nv_block_ops *ops,
			      void *priv, sector_t capacity_sectors,
			      unsigned int logical_block_size)
{
	struct queue_limits lim = { };
	int ret;

	if (!drv || !dev || !name || !ops || !ops->submit_bio ||
	    index >= drv->count || !capacity_sectors)
		return -EINVAL;

	if (dev->registered)
		return -EBUSY;

	if (!logical_block_size)
		logical_block_size = 512;

	dev->driver = drv;
	dev->index = index;
	dev->name = name;
	dev->ops = ops;
	dev->priv = priv;
	dev->capacity = capacity_sectors;
	dev->logical_block_size = logical_block_size;

	lim.logical_block_size = logical_block_size;
	lim.physical_block_size = logical_block_size;
	lim.max_hw_sectors = BLK_DEF_MAX_SECTORS_CAP;
	lim.max_sectors = BLK_DEF_MAX_SECTORS_CAP;

	dev->disk = blk_alloc_disk(&lim, NUMA_NO_NODE);
	if (IS_ERR(dev->disk))
		return PTR_ERR(dev->disk);

	dev->disk->fops = &nv_block_bdops;
	dev->disk->private_data = dev;
	strscpy(dev->disk->disk_name, name, DISK_NAME_LEN);
	set_capacity(dev->disk, capacity_sectors);

	ret = add_disk(dev->disk);
	if (ret) {
		put_disk(dev->disk);
		dev->disk = NULL;
		return ret;
	}

	drv->devices[index] = dev;
	dev->registered = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_block_device_register);

void nv_block_device_unregister(struct nv_block_dev *dev)
{
	if (!dev || !dev->registered)
		return;

	if (dev->driver && dev->driver->devices)
		dev->driver->devices[dev->index] = NULL;

	if (dev->disk) {
		del_gendisk(dev->disk);
		put_disk(dev->disk);
		dev->disk = NULL;
	}

	dev->registered = false;
}
EXPORT_SYMBOL_GPL(nv_block_device_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("Block device framework for LinuxDriverFramework");
MODULE_LICENSE("GPL");
