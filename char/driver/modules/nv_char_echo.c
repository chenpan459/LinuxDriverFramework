// SPDX-License-Identifier: GPL-2.0
/*
 * Example: echo character device using nv_char framework
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "nv_char.h"

#define ECHO_BUF_SIZE	4096
#define ECHO_DEV_NAME	"nv_echo"

struct echo_priv {
	char *buf;
	size_t len;
	struct mutex lock;
};

static struct nv_char_driver echo_driver;
static struct nv_char_dev echo_dev;
static struct echo_priv echo_data;

static ssize_t echo_read(struct nv_char_dev *dev, struct file *filp,
			 char __user *buf, size_t count, loff_t *ppos)
{
	struct echo_priv *priv = dev->priv;
	size_t avail;
	ssize_t ret;

	if (!priv || *ppos < 0)
		return -EINVAL;

	mutex_lock(&priv->lock);
	avail = priv->len;
	if (*ppos >= avail) {
		ret = 0;
		goto out;
	}
	avail -= *ppos;
	if (count > avail)
		count = avail;

	if (copy_to_user(buf, priv->buf + *ppos, count)) {
		ret = -EFAULT;
		goto out;
	}

	*ppos += count;
	ret = count;
out:
	mutex_unlock(&priv->lock);
	return ret;
}

static ssize_t echo_write(struct nv_char_dev *dev, struct file *filp,
			  const char __user *buf, size_t count, loff_t *ppos)
{
	struct echo_priv *priv = dev->priv;
	ssize_t ret;

	if (!priv)
		return -EINVAL;

	if (count > ECHO_BUF_SIZE)
		count = ECHO_BUF_SIZE;

	mutex_lock(&priv->lock);
	if (copy_from_user(priv->buf, buf, count)) {
		ret = -EFAULT;
		goto out;
	}

	priv->len = count;
	*ppos = count;
	ret = count;
	nv_char_wake(dev);
out:
	mutex_unlock(&priv->lock);
	return ret;
}

static const struct nv_char_ops echo_ops = {
	.read	= echo_read,
	.write	= echo_write,
};

static int __init echo_init(void)
{
	int ret;

	echo_data.buf = kzalloc(ECHO_BUF_SIZE, GFP_KERNEL);
	if (!echo_data.buf)
		return -ENOMEM;
	mutex_init(&echo_data.lock);

	echo_driver.name = "nv_char_echo";
	echo_driver.owner = THIS_MODULE;
	echo_driver.count = 1;
	echo_driver.dev_mode = 0660;

	ret = nv_char_driver_register(&echo_driver);
	if (ret)
		goto err_buf;

	ret = nv_char_device_register(&echo_driver, &echo_dev, 0,
				      ECHO_DEV_NAME, &echo_ops, &echo_data);
	if (ret)
		goto err_drv;

	pr_info("nv_char_echo: cdev ready major=%u minor=%u /dev/%s\n",
		nv_char_dev_major(&echo_dev), nv_char_dev_minor(&echo_dev),
		ECHO_DEV_NAME);
	return 0;

err_drv:
	nv_char_driver_unregister(&echo_driver);
err_buf:
	kfree(echo_data.buf);
	return ret;
}

static void __exit echo_exit(void)
{
	nv_char_device_unregister(&echo_dev);
	nv_char_driver_unregister(&echo_driver);
	kfree(echo_data.buf);
	pr_info("nv_char_echo: unloaded\n");
}

module_init(echo_init);
module_exit(echo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("Echo character device example");
MODULE_LICENSE("GPL");
