// SPDX-License-Identifier: GPL-2.0
/*
 * Example: miscdevice (dynamic minor) via nv_char_misc_register
 *
 * Load:  insmod nv_char_misc_echo.ko
 * Test:  echo hi > /dev/nv_misc && cat /dev/nv_misc
 * Unload: rmmod nv_char_misc_echo
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "nv_char.h"

#define MISC_NAME	"nv_misc"

struct misc_echo_priv {
	char *buf;
	size_t len;
	struct mutex lock;
};

static struct nv_char_misc_dev misc_dev;
static struct misc_echo_priv echo_data;

static ssize_t misc_read(struct nv_char_dev *dev, struct file *filp,
			 char __user *buf, size_t count, loff_t *ppos)
{
	struct misc_echo_priv *priv = dev->priv;
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

static ssize_t misc_write(struct nv_char_dev *dev, struct file *filp,
			  const char __user *buf, size_t count, loff_t *ppos)
{
	struct misc_echo_priv *priv = dev->priv;
	ssize_t ret;

	if (!priv)
		return -EINVAL;

	if (count > PAGE_SIZE)
		count = PAGE_SIZE;

	mutex_lock(&priv->lock);
	if (copy_from_user(priv->buf, buf, count)) {
		ret = -EFAULT;
		goto out;
	}

	priv->len = count;
	*ppos = count;
	ret = count;
out:
	mutex_unlock(&priv->lock);
	return ret;
}

static const struct nv_char_ops misc_ops = {
	.read	= misc_read,
	.write	= misc_write,
};

static int __init misc_echo_init(void)
{
	int ret;

	echo_data.buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!echo_data.buf)
		return -ENOMEM;
	mutex_init(&echo_data.lock);

	ret = nv_char_misc_register(&misc_dev, MISC_NAME, &misc_ops,
				    &echo_data, THIS_MODULE);
	if (ret)
		goto err_buf;

	pr_info("nv_char_misc_echo: misc ready /dev/%s\n", MISC_NAME);
	return 0;

err_buf:
	kfree(echo_data.buf);
	return ret;
}

static void __exit misc_echo_exit(void)
{
	nv_char_misc_unregister(&misc_dev);
	kfree(echo_data.buf);
	pr_info("nv_char_misc_echo: unloaded\n");
}

module_init(misc_echo_init);
module_exit(misc_echo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("Miscdevice echo example for nv_char");
MODULE_LICENSE("GPL");
