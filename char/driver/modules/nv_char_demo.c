// SPDX-License-Identifier: GPL-2.0
/*
 * Example: poll + ioctl dispatch using nv_char framework
 *
 * Load:  insmod nv_char_demo.ko
 * Test:  cat /dev/nv_demo & sleep 1; echo hi > /dev/nv_demo
 *        ioctl: python/cpp tool or custom app with NV_DEMO_IOC_GET_LEN
 * Unload: rmmod nv_char_demo
 */

#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "nv_char.h"

#define DEMO_DEV_NAME	"nv_demo"
#define DEMO_BUF_SIZE	256

#define NV_DEMO_IOC_MAGIC	'v'
#define NV_DEMO_IOC_GET_LEN	_IOR(NV_DEMO_IOC_MAGIC, 0x01, int)
#define NV_DEMO_IOC_CLEAR	_IO(NV_DEMO_IOC_MAGIC, 0x02)

struct demo_priv {
	char *buf;
	size_t len;
	bool data_ready;
	struct mutex lock;
};

static struct nv_char_driver demo_driver;
static struct nv_char_dev demo_dev;
static struct demo_priv demo_data;

static ssize_t demo_read(struct nv_char_dev *dev, struct file *filp,
			 char __user *buf, size_t count, loff_t *ppos)
{
	struct demo_priv *priv = dev->priv;
	ssize_t ret;

	if (!priv)
		return -EINVAL;

	mutex_lock(&priv->lock);
	if (!priv->data_ready || !priv->len) {
		ret = 0;
		goto out;
	}
	if (*ppos >= priv->len) {
		ret = 0;
		goto out;
	}
	if (count > priv->len - *ppos)
		count = priv->len - *ppos;

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

static ssize_t demo_write(struct nv_char_dev *dev, struct file *filp,
			  const char __user *buf, size_t count, loff_t *ppos)
{
	struct demo_priv *priv = dev->priv;
	ssize_t ret;

	if (!priv)
		return -EINVAL;

	if (count > DEMO_BUF_SIZE)
		count = DEMO_BUF_SIZE;

	mutex_lock(&priv->lock);
	if (copy_from_user(priv->buf, buf, count)) {
		ret = -EFAULT;
		goto out;
	}

	priv->len = count;
	priv->data_ready = true;
	*ppos = 0;
	ret = count;
	nv_char_wake(dev);
out:
	mutex_unlock(&priv->lock);
	return ret;
}

static __poll_t demo_poll(struct nv_char_dev *dev, struct file *filp,
			  poll_table *wait)
{
	struct demo_priv *priv = dev->priv;
	__poll_t mask = 0;

	if (!priv)
		return EPOLLERR;

	poll_wait(filp, &dev->wq, wait);

	mutex_lock(&priv->lock);
	if (priv->data_ready && priv->len)
		mask |= EPOLLIN | EPOLLRDNORM;
	mutex_unlock(&priv->lock);

	mask |= EPOLLOUT | EPOLLWRNORM;
	return mask;
}

static int demo_ioctl_get_len(struct nv_char_dev *dev, struct file *filp,
			      unsigned long arg)
{
	struct demo_priv *priv = dev->priv;
	int len;

	if (!priv)
		return -EINVAL;

	mutex_lock(&priv->lock);
	len = priv->len;
	mutex_unlock(&priv->lock);

	if (copy_to_user((void __user *)arg, &len, sizeof(len)))
		return -EFAULT;

	return 0;
}

static int demo_ioctl_clear(struct nv_char_dev *dev, struct file *filp,
			    unsigned long arg)
{
	struct demo_priv *priv = dev->priv;

	if (!priv)
		return -EINVAL;

	mutex_lock(&priv->lock);
	priv->len = 0;
	priv->data_ready = false;
	mutex_unlock(&priv->lock);
	nv_char_wake(dev);
	return 0;
}

static const struct nv_char_ioctl_entry demo_ioctls[] = {
	{ NV_DEMO_IOC_GET_LEN, demo_ioctl_get_len, 0 },
	{ NV_DEMO_IOC_CLEAR,   demo_ioctl_clear,   0 },
};

static long demo_ioctl(struct nv_char_dev *dev, struct file *filp,
		       unsigned int cmd, unsigned long arg)
{
	return nv_char_ioctl_dispatch(dev, filp, cmd, arg,
				      demo_ioctls,
				      ARRAY_SIZE(demo_ioctls));
}

static const struct nv_char_ops demo_ops = {
	.read		= demo_read,
	.write		= demo_write,
	.poll		= demo_poll,
	.unlocked_ioctl	= demo_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= demo_ioctl,
#endif
};

static int __init demo_init(void)
{
	int ret;

	demo_data.buf = kzalloc(DEMO_BUF_SIZE, GFP_KERNEL);
	if (!demo_data.buf)
		return -ENOMEM;
	mutex_init(&demo_data.lock);

	demo_driver.name = "nv_char_demo";
	demo_driver.owner = THIS_MODULE;
	demo_driver.count = 1;
	demo_driver.dev_mode = 0666;

	ret = nv_char_driver_register(&demo_driver);
	if (ret)
		goto err_buf;

	ret = nv_char_device_register(&demo_driver, &demo_dev, 0,
				      DEMO_DEV_NAME, &demo_ops, &demo_data);
	if (ret)
		goto err_drv;

	pr_info("nv_char_demo: ready /dev/%s (poll+ioctl)\n", DEMO_DEV_NAME);
	return 0;

err_drv:
	nv_char_driver_unregister(&demo_driver);
err_buf:
	kfree(demo_data.buf);
	return ret;
}

static void __exit demo_exit(void)
{
	nv_char_device_unregister(&demo_dev);
	nv_char_driver_unregister(&demo_driver);
	kfree(demo_data.buf);
	pr_info("nv_char_demo: unloaded\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("Poll and ioctl demo for nv_char");
MODULE_LICENSE("GPL");
