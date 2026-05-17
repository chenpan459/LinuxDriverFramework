// SPDX-License-Identifier: GPL-2.0
/*
 * Example: RAM-backed block device using nv_block framework
 *
 * Load:  insmod nv_block_ram.ko [size_mb=8]
 * Test:  dd if=/dev/zero of=/dev/nv_ram bs=4K count=1
 *         dd if=/dev/nv_ram of=/tmp/out bs=4K count=1
 * Unload: rmmod nv_block_ram
 */

#include <linux/bio.h>
#include <linux/bvec.h>
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "nv_block.h"

#define RAM_DISK_NAME	"nv_ram"
#define RAM_DEFAULT_MB	8
#define RAM_BLOCK_SIZE	512

struct ram_priv {
	void *data;
	sector_t sectors;
	spinlock_t lock;
};

static struct nv_block_driver ram_driver;
static struct nv_block_dev ram_dev;
static struct ram_priv ram_data;

static unsigned long ram_size_mb = RAM_DEFAULT_MB;
module_param(ram_size_mb, ulong, 0444);
MODULE_PARM_DESC(ram_size_mb, "RAM disk size in megabytes (default 8)");

static blk_status_t ram_submit_bio(struct nv_block_dev *dev, struct bio *bio)
{
	struct ram_priv *priv = dev->priv;
	struct bio_vec bvec;
	struct bvec_iter iter;
	unsigned long flags;

	if (!priv || !priv->data)
		return BLK_STS_IOERR;

	bio_for_each_segment(bvec, bio, iter) {
		sector_t start = iter.bi_sector;
		size_t len = bvec.bv_len;
		void *buf = bvec_virt(&bvec);
		size_t offset = start * RAM_BLOCK_SIZE;

		if (start + (len >> 9) > priv->sectors)
			return BLK_STS_IOERR;

		spin_lock_irqsave(&priv->lock, flags);
		if (bio_data_dir(bio) == WRITE)
			memcpy(priv->data + offset, buf, len);
		else
			memcpy(buf, priv->data + offset, len);
		spin_unlock_irqrestore(&priv->lock, flags);
	}

	return BLK_STS_OK;
}

static const struct nv_block_ops ram_ops = {
	.submit_bio = ram_submit_bio,
};

static int __init ram_init(void)
{
	int ret;
	sector_t sectors;

	if (!ram_size_mb || ram_size_mb > 1024)
		ram_size_mb = RAM_DEFAULT_MB;

	sectors = (ram_size_mb * 1024 * 1024) / RAM_BLOCK_SIZE;
	if (!sectors)
		return -EINVAL;

	ram_data.sectors = sectors;
	ram_data.data = kzalloc(sectors * RAM_BLOCK_SIZE, GFP_KERNEL);
	if (!ram_data.data)
		return -ENOMEM;
	spin_lock_init(&ram_data.lock);

	ram_driver.name = "nv_block_ram";
	ram_driver.owner = THIS_MODULE;
	ram_driver.count = 1;

	ret = nv_block_driver_register(&ram_driver);
	if (ret)
		goto err_data;

	ret = nv_block_device_register(&ram_driver, &ram_dev, 0, RAM_DISK_NAME,
					&ram_ops, &ram_data, sectors,
					RAM_BLOCK_SIZE);
	if (ret)
		goto err_drv;

	pr_info("nv_block_ram: ready (/dev/%s, %lu MB)\n", RAM_DISK_NAME,
		(unsigned long)ram_size_mb);
	return 0;

err_drv:
	nv_block_driver_unregister(&ram_driver);
err_data:
	kfree(ram_data.data);
	return ret;
}

static void __exit ram_exit(void)
{
	nv_block_device_unregister(&ram_dev);
	nv_block_driver_unregister(&ram_driver);
	kfree(ram_data.data);
	pr_info("nv_block_ram: unloaded\n");
}

module_init(ram_init);
module_exit(ram_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("RAM block device example");
MODULE_LICENSE("GPL");
