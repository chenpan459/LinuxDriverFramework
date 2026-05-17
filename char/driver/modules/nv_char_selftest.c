// SPDX-License-Identifier: GPL-2.0
/*
 * In-kernel selftest for nv_char (loads only if all checks pass)
 *
 * Load:  insmod nv_char_selftest.ko
 *        dmesg | grep nv_char_selftest
 * Unload: rmmod nv_char_selftest
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "nv_char.h"

#define SELFTEST_NAME	"nv_selftest"

struct st_priv {
	struct mutex lock;
	int value;
};

static struct nv_char_driver st_drv;
static struct nv_char_dev st_dev;
static struct st_priv st_data;

static ssize_t st_read(struct nv_char_dev *dev, struct file *filp,
		       char __user *buf, size_t count, loff_t *ppos)
{
	struct st_priv *priv = dev->priv;
	int val;
	char tmp[32];
	int len;

	if (!priv)
		return -EINVAL;

	mutex_lock(&priv->lock);
	val = priv->value;
	mutex_unlock(&priv->lock);

	len = scnprintf(tmp, sizeof(tmp), "%d\n", val);
	if (*ppos >= len)
		return 0;
	if (count > len - *ppos)
		count = len - *ppos;
	if (copy_to_user(buf, tmp + *ppos, count))
		return -EFAULT;
	*ppos += count;
	return count;
}

static const struct nv_char_ops st_ops = {
	.read = st_read,
};

static int __init selftest_init(void)
{
	int ret;
	unsigned int maj, min;

	mutex_init(&st_data.lock);
	st_data.value = 42;

	st_drv.name = "nv_char_selftest";
	st_drv.owner = THIS_MODULE;
	st_drv.count = 1;

	ret = nv_char_driver_register(&st_drv);
	if (ret) {
		pr_err("nv_char_selftest: driver_register failed %d\n", ret);
		return ret;
	}

	ret = nv_char_device_register(&st_drv, &st_dev, 0, SELFTEST_NAME,
				      &st_ops, &st_data);
	if (ret) {
		pr_err("nv_char_selftest: device_register failed %d\n", ret);
		nv_char_driver_unregister(&st_drv);
		return ret;
	}

	maj = nv_char_dev_major(&st_dev);
	min = nv_char_dev_minor(&st_dev);
	if (!maj && !min) {
		pr_err("nv_char_selftest: invalid devt\n");
		nv_char_device_unregister(&st_dev);
		nv_char_driver_unregister(&st_drv);
		return -EINVAL;
	}

	pr_info("nv_char_selftest: PASS (major=%u minor=%u /dev/%s)\n",
		maj, min, SELFTEST_NAME);
	return 0;
}

static void __exit selftest_exit(void)
{
	nv_char_device_unregister(&st_dev);
	nv_char_driver_unregister(&st_drv);
	pr_info("nv_char_selftest: done\n");
}

module_init(selftest_init);
module_exit(selftest_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("nv_char framework selftest");
MODULE_LICENSE("GPL");
