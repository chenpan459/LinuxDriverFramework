// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - character device helper implementation
 */

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "nv_char.h"

static struct nv_char_dev *nv_char_dev_from_file(struct file *filp)
{
	return filp->private_data;
}

static int nv_char_open(struct inode *inode, struct file *filp)
{
	struct nv_char_dev *dev;

	dev = container_of(inode->i_cdev, struct nv_char_dev, cdev);
	filp->private_data = dev;

	if (dev->ops && dev->ops->open)
		return dev->ops->open(dev, inode, filp);

	return 0;
}

static int nv_char_release(struct inode *inode, struct file *filp)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (dev->ops && dev->ops->release)
		return dev->ops->release(dev, inode, filp);

	return 0;
}

static ssize_t nv_char_read(struct file *filp, char __user *buf, size_t count,
			     loff_t *ppos)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (dev->ops && dev->ops->read)
		return dev->ops->read(dev, filp, buf, count, ppos);

	return -ENODEV;
}

static ssize_t nv_char_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (dev->ops && dev->ops->write)
		return dev->ops->write(dev, filp, buf, count, ppos);

	return -ENODEV;
}

static long nv_char_unlocked_ioctl(struct file *filp, unsigned int cmd,
				    unsigned long arg)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (dev->ops && dev->ops->unlocked_ioctl)
		return dev->ops->unlocked_ioctl(dev, filp, cmd, arg);

	return -ENOTTY;
}

static long nv_char_compat_ioctl(struct file *filp, unsigned int cmd,
				  unsigned long arg)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (dev->ops && dev->ops->compat_ioctl)
		return dev->ops->compat_ioctl(dev, filp, cmd, arg);

	return -ENOTTY;
}

static __poll_t nv_char_poll(struct file *filp, poll_table *wait)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (dev->ops && dev->ops->poll)
		return dev->ops->poll(dev, filp, wait);

	return EPOLLERR;
}

static loff_t nv_char_llseek(struct file *filp, loff_t offset, int whence)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (dev->ops && dev->ops->llseek)
		return dev->ops->llseek(dev, filp, offset, whence);

	return default_llseek(filp, offset, whence);
}

static const struct file_operations nv_char_fops = {
	.owner		= THIS_MODULE,
	.open		= nv_char_open,
	.release	= nv_char_release,
	.read		= nv_char_read,
	.write		= nv_char_write,
	.unlocked_ioctl	= nv_char_unlocked_ioctl,
	.compat_ioctl	= nv_char_compat_ioctl,
	.poll		= nv_char_poll,
	.llseek		= nv_char_llseek,
};

int nv_char_driver_register(struct nv_char_driver *drv)
{
	int ret;

	if (!drv || !drv->name || !drv->count || !drv->owner)
		return -EINVAL;

	drv->devices = kcalloc(drv->count, sizeof(*drv->devices), GFP_KERNEL);
	if (!drv->devices)
		return -ENOMEM;

	ret = alloc_chrdev_region(&drv->devt, 0, drv->count, drv->name);
	if (ret)
		goto err_devices;

	drv->class = class_create(drv->name);
	if (IS_ERR(drv->class)) {
		ret = PTR_ERR(drv->class);
		goto err_region;
	}

	return 0;

err_region:
	unregister_chrdev_region(drv->devt, drv->count);
err_devices:
	kfree(drv->devices);
	drv->devices = NULL;
	return ret;
}
EXPORT_SYMBOL_GPL(nv_char_driver_register);

void nv_char_driver_unregister(struct nv_char_driver *drv)
{
	unsigned int i;

	if (!drv)
		return;

	if (drv->devices) {
		for (i = 0; i < drv->count; i++) {
			if (drv->devices[i])
				nv_char_device_unregister(drv->devices[i]);
		}
		kfree(drv->devices);
		drv->devices = NULL;
	}

	if (drv->class && !IS_ERR(drv->class))
		class_destroy(drv->class);
	drv->class = NULL;

	if (drv->devt)
		unregister_chrdev_region(drv->devt, drv->count);
	drv->devt = 0;
}
EXPORT_SYMBOL_GPL(nv_char_driver_unregister);

int nv_char_device_register(struct nv_char_driver *drv,
			     struct nv_char_dev *dev, unsigned int index,
			     const char *name, const struct nv_char_ops *ops,
			     void *priv)
{
	int ret;

	if (!drv || !dev || !name || index >= drv->count)
		return -EINVAL;

	if (dev->registered)
		return -EBUSY;

	dev->driver = drv;
	dev->index = index;
	dev->name = name;
	dev->ops = ops;
	dev->priv = priv;
	dev->devt = MKDEV(MAJOR(drv->devt), MINOR(drv->devt) + index);

	cdev_init(&dev->cdev, &nv_char_fops);
	dev->cdev.owner = drv->owner;

	ret = cdev_add(&dev->cdev, dev->devt, 1);
	if (ret)
		return ret;

	dev->device = device_create(drv->class, NULL, dev->devt, dev, "%s",
				    name);
	if (IS_ERR(dev->device)) {
		ret = PTR_ERR(dev->device);
		dev->device = NULL;
		cdev_del(&dev->cdev);
		return ret;
	}

	drv->devices[index] = dev;
	dev->registered = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_char_device_register);

void nv_char_device_unregister(struct nv_char_dev *dev)
{
	if (!dev || !dev->registered)
		return;

	if (dev->driver && dev->driver->devices)
		dev->driver->devices[dev->index] = NULL;

	if (dev->device) {
		device_destroy(dev->driver->class, dev->devt);
		dev->device = NULL;
	}

	cdev_del(&dev->cdev);
	dev->registered = false;
}
EXPORT_SYMBOL_GPL(nv_char_device_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("Character device framework for LinuxDriverFramework");
MODULE_LICENSE("GPL");
