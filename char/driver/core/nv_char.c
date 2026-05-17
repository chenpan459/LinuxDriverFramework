// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - character device helper (cdev)
 */

#include <linux/capability.h>
#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/uio.h>

#include "nv_char.h"
#include <linux/device/devres.h>

#define CREATE_TRACE_POINTS
#include <trace/events/nv_char.h>

#define NV_CHAR_UNREG_TIMEOUT	(10 * HZ)

struct nv_char_chrdev_dr {
	dev_t devt;
	unsigned int count;
	unsigned int major;
};

static struct dentry *nv_char_debugfs_root;
static int nv_char_debugfs_refcnt;

static struct nv_char_dev *nv_char_dev_from_file(struct file *filp)
{
	return filp->private_data;
}

static bool nv_char_is_removing(struct nv_char_dev *dev)
{
	return dev && READ_ONCE(dev->removing);
}

static struct module *nv_char_dev_owner(struct nv_char_dev *dev)
{
	if (dev->owner)
		return dev->owner;
	if (dev->driver)
		return dev->driver->owner;
	return NULL;
}

static int nv_char_acquire_open(struct nv_char_dev *dev)
{
	struct module *owner;

	if (nv_char_is_removing(dev))
		return -ENODEV;

	owner = nv_char_dev_owner(dev);
	if (!owner || !try_module_get(owner))
		return -ENODEV;

	if (nv_char_is_removing(dev)) {
		module_put(owner);
		return -ENODEV;
	}

	atomic_inc(&dev->opens);
	return 0;
}

static void nv_char_release_open(struct nv_char_dev *dev)
{
	struct module *owner = nv_char_dev_owner(dev);

	if (atomic_dec_and_test(&dev->opens))
		wake_up_all(&dev->release_wq);
	if (owner)
		module_put(owner);
}

static int nv_char_open(struct inode *inode, struct file *filp)
{
	struct nv_char_dev *dev;
	int ret;

	dev = container_of(inode->i_cdev, struct nv_char_dev, cdev);

	ret = nv_char_acquire_open(dev);
	if (ret)
		return ret;

	filp->private_data = dev;

	if (dev->ops && dev->ops->open) {
		ret = dev->ops->open(dev, inode, filp);
		if (ret) {
			filp->private_data = NULL;
			nv_char_release_open(dev);
			return ret;
		}
	}

	trace_nv_char_open(dev->devt, atomic_read(&dev->opens));
	return 0;
}

static int nv_char_release(struct inode *inode, struct file *filp)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);
	int ret = 0;

	if (!dev)
		return 0;

	if (dev->ops && dev->ops->release)
		ret = dev->ops->release(dev, inode, filp);

	filp->private_data = NULL;
	nv_char_release_open(dev);
	trace_nv_char_release(dev->devt, atomic_read(&dev->opens));
	return ret;
}

static ssize_t nv_char_read(struct file *filp, char __user *buf, size_t count,
			    loff_t *ppos)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (nv_char_is_removing(dev))
		return -ENODEV;

	if (dev->ops && dev->ops->read)
		return dev->ops->read(dev, filp, buf, count, ppos);

	return -ENOSYS;
}

static ssize_t nv_char_write(struct file *filp, const char __user *buf,
			     size_t count, loff_t *ppos)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (nv_char_is_removing(dev))
		return -ENODEV;

	if (dev->ops && dev->ops->write)
		return dev->ops->write(dev, filp, buf, count, ppos);

	return -ENOSYS;
}

static ssize_t nv_char_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	struct file *filp = iocb->ki_filp;
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);
	ssize_t ret;
	size_t count;
	void *kbuf;

	if (nv_char_is_removing(dev))
		return -ENODEV;

	if (dev->ops && dev->ops->read_iter)
		return dev->ops->read_iter(dev, iocb, iter);

	if (!dev->ops || !dev->ops->read)
		return -ENOSYS;

	count = iov_iter_count(iter);
	if (!count)
		return 0;

	kbuf = kvmalloc(count, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;

	ret = dev->ops->read(dev, filp, kbuf, count, &iocb->ki_pos);
	if (ret > 0) {
		if (copy_to_iter(kbuf, ret, iter) != ret)
			ret = -EFAULT;
	}

	kvfree(kbuf);
	return ret;
}

static ssize_t nv_char_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	struct file *filp = iocb->ki_filp;
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);
	ssize_t ret;
	size_t count;
	void *kbuf;

	if (nv_char_is_removing(dev))
		return -ENODEV;

	if (dev->ops && dev->ops->write_iter)
		return dev->ops->write_iter(dev, iocb, iter);

	if (!dev->ops || !dev->ops->write)
		return -ENOSYS;

	count = iov_iter_count(iter);
	if (!count)
		return 0;

	kbuf = kvmalloc(count, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;

	if (!copy_from_iter(kbuf, count, iter)) {
		kvfree(kbuf);
		return -EFAULT;
	}

	ret = dev->ops->write(dev, filp, kbuf, count, &iocb->ki_pos);
	kvfree(kbuf);
	return ret;
}

static long nv_char_unlocked_ioctl(struct file *filp, unsigned int cmd,
				   unsigned long arg)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (nv_char_is_removing(dev))
		return -ENODEV;

	if (dev->ops && dev->ops->unlocked_ioctl)
		return dev->ops->unlocked_ioctl(dev, filp, cmd, arg);

	return -ENOTTY;
}

static long nv_char_compat_ioctl(struct file *filp, unsigned int cmd,
				 unsigned long arg)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (nv_char_is_removing(dev))
		return -ENODEV;

	if (dev->ops && dev->ops->compat_ioctl)
		return dev->ops->compat_ioctl(dev, filp, cmd, arg);

#ifdef CONFIG_COMPAT
	if (dev->ops && dev->ops->unlocked_ioctl)
		return dev->ops->unlocked_ioctl(dev, filp, cmd, arg);
#endif

	return -ENOTTY;
}

static __poll_t nv_char_poll(struct file *filp, poll_table *wait)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (nv_char_is_removing(dev))
		return EPOLLERR | EPOLLHUP;

	poll_wait(filp, &dev->wq, wait);

	if (dev->ops && dev->ops->poll)
		return dev->ops->poll(dev, filp, wait);

	return 0;
}

static loff_t nv_char_llseek(struct file *filp, loff_t offset, int whence)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (nv_char_is_removing(dev))
		return -ENODEV;

	if (dev->ops && dev->ops->llseek)
		return dev->ops->llseek(dev, filp, offset, whence);

	return default_llseek(filp, offset, whence);
}

static int nv_char_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (nv_char_is_removing(dev))
		return -ENODEV;

	if (dev->ops && dev->ops->mmap)
		return dev->ops->mmap(dev, filp, vma);

	return -ENODEV;
}

static int nv_char_fsync(struct file *filp, loff_t start, loff_t end,
			 int datasync)
{
	struct nv_char_dev *dev = nv_char_dev_from_file(filp);

	if (nv_char_is_removing(dev))
		return -ENODEV;

	if (dev->ops && dev->ops->fsync)
		return dev->ops->fsync(dev, filp, start, end, datasync);

	return 0;
}

static char *nv_char_devnode(const struct device *d, umode_t *mode)
{
	struct nv_char_dev *dev = dev_get_drvdata(d);

	if (dev && dev->dev_mode)
		*mode = dev->dev_mode;
	else if (dev && dev->driver && dev->driver->dev_mode)
		*mode = dev->driver->dev_mode;

	return NULL;
}

static ssize_t opens_show(struct device *d, struct device_attribute *attr,
			  char *buf)
{
	struct nv_char_dev *dev = dev_get_drvdata(d);

	return sysfs_emit(buf, "%d\n", atomic_read(&dev->opens));
}
static DEVICE_ATTR_RO(opens);

long nv_char_ioctl_dispatch(struct nv_char_dev *dev, struct file *filp,
			    unsigned int cmd, unsigned long arg,
			    const struct nv_char_ioctl_entry *table,
			    unsigned int nent)
{
	unsigned int i;

	if (!table)
		return -ENOTTY;

	for (i = 0; i < nent; i++) {
		if (table[i].cmd != cmd)
			continue;
		if (table[i].capable && !capable(table[i].capable))
			return -EPERM;
		if (table[i].handler)
			return table[i].handler(dev, filp, arg);
		return -ENOTTY;
	}

	return -ENOTTY;
}
EXPORT_SYMBOL_GPL(nv_char_ioctl_dispatch);

void nv_char_wake(struct nv_char_dev *dev)
{
	if (dev)
		wake_up_interruptible(&dev->wq);
}
EXPORT_SYMBOL_GPL(nv_char_wake);

#define NV_CHAR_FOPS_COMMON						\
	.release	= nv_char_release,				\
	.read		= nv_char_read,					\
	.write		= nv_char_write,				\
	.read_iter	= nv_char_read_iter,				\
	.write_iter	= nv_char_write_iter,				\
	.unlocked_ioctl	= nv_char_unlocked_ioctl,			\
	.compat_ioctl	= nv_char_compat_ioctl,				\
	.poll		= nv_char_poll,					\
	.llseek		= nv_char_llseek,				\
	.mmap		= nv_char_mmap,					\
	.fsync		= nv_char_fsync

static int nv_char_misc_open(struct inode *inode, struct file *filp)
{
	struct miscdevice *misc = filp->private_data;
	struct nv_char_misc_dev *m;
	struct nv_char_dev *dev;
	int ret;

	m = container_of(misc, struct nv_char_misc_dev, misc);
	dev = &m->dev;

	ret = nv_char_acquire_open(dev);
	if (ret)
		return ret;

	filp->private_data = dev;

	if (dev->ops && dev->ops->open) {
		ret = dev->ops->open(dev, inode, filp);
		if (ret) {
			filp->private_data = NULL;
			nv_char_release_open(dev);
			return ret;
		}
	}

	trace_nv_char_open(dev->devt, atomic_read(&dev->opens));
	return 0;
}

static const struct file_operations nv_char_fops = {
	.owner		= THIS_MODULE,
	.open		= nv_char_open,
	NV_CHAR_FOPS_COMMON,
};

static const struct file_operations nv_char_misc_fops = {
	.owner		= THIS_MODULE,
	.open		= nv_char_misc_open,
	NV_CHAR_FOPS_COMMON,
};

static void nv_char_debugfs_add_device(struct nv_char_dev *dev)
{
	if (!nv_char_debugfs_root || !dev || !dev->name)
		return;

	dev->debugfs_dir = debugfs_create_dir(dev->name, nv_char_debugfs_root);
	if (IS_ERR(dev->debugfs_dir))
		dev->debugfs_dir = NULL;

	if (dev->debugfs_dir) {
		debugfs_create_atomic_t("opens", 0444, dev->debugfs_dir,
					&dev->opens);
		debugfs_create_bool("removing", 0444, dev->debugfs_dir,
				    &dev->removing);
		debugfs_create_u32("devt", 0444, dev->debugfs_dir,
				   (u32 *)&dev->devt);
	}
}

static void nv_char_debugfs_remove_device(struct nv_char_dev *dev)
{
	if (dev && dev->debugfs_dir) {
		debugfs_remove_recursive(dev->debugfs_dir);
		dev->debugfs_dir = NULL;
	}
}

void nv_char_debugfs_init(void)
{
	if (nv_char_debugfs_refcnt++)
		return;

	nv_char_debugfs_root = debugfs_create_dir("nv_char", NULL);
	if (IS_ERR(nv_char_debugfs_root)) {
		nv_char_debugfs_root = NULL;
		nv_char_debugfs_refcnt = 0;
	}
}
EXPORT_SYMBOL_GPL(nv_char_debugfs_init);

void nv_char_debugfs_exit(void)
{
	if (nv_char_debugfs_refcnt == 0)
		return;

	if (--nv_char_debugfs_refcnt == 0) {
		debugfs_remove_recursive(nv_char_debugfs_root);
		nv_char_debugfs_root = NULL;
	}
}
EXPORT_SYMBOL_GPL(nv_char_debugfs_exit);

int nv_char_driver_register(struct nv_char_driver *drv)
{
	int ret;

	if (!drv || !drv->name || !drv->count || !drv->owner)
		return -EINVAL;

	drv->devices = kcalloc(drv->count, sizeof(*drv->devices), GFP_KERNEL);
	if (!drv->devices)
		return -ENOMEM;

	if (drv->major)
		ret = register_chrdev_region(MKDEV(drv->major, 0), drv->count,
					     drv->name);
	else
		ret = alloc_chrdev_region(&drv->devt, 0, drv->count, drv->name);
	if (ret)
		goto err_devices;

	if (drv->major)
		drv->devt = MKDEV(drv->major, 0);

	drv->class = class_create(drv->name);
	if (IS_ERR(drv->class)) {
		ret = PTR_ERR(drv->class);
		goto err_region;
	}

	drv->class->devnode = nv_char_devnode;
	nv_char_debugfs_init();

	return 0;

err_region:
	if (drv->major)
		unregister_chrdev_region(MKDEV(drv->major, 0), drv->count);
	else
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
		if (!drv->devm_managed)
			kfree(drv->devices);
		drv->devices = NULL;
	}

	nv_char_debugfs_exit();

	if (!drv->devm_managed) {
		if (drv->class && !IS_ERR(drv->class))
			class_destroy(drv->class);
		if (drv->devt) {
			if (drv->major)
				unregister_chrdev_region(MKDEV(drv->major, 0),
							 drv->count);
			else
				unregister_chrdev_region(drv->devt, drv->count);
		}
	}
	drv->class = NULL;
	drv->devt = 0;
	drv->devm_managed = false;
	drv->devm_dev = NULL;
}
EXPORT_SYMBOL_GPL(nv_char_driver_unregister);

static void nv_char_devm_chrdev_release(void *data)
{
	struct nv_char_chrdev_dr *dr = data;

	if (!dr)
		return;

	if (dr->major)
		unregister_chrdev_region(MKDEV(dr->major, 0), dr->count);
	else if (dr->devt)
		unregister_chrdev_region(dr->devt, dr->count);
}

static void nv_char_devm_class_release(void *data)
{
	struct nv_char_driver *drv = data;

	if (drv && drv->class && !IS_ERR(drv->class)) {
		class_destroy(drv->class);
		drv->class = NULL;
	}
}

int nv_char_driver_register_devm(struct device *dev, struct nv_char_driver *drv)
{
	struct nv_char_chrdev_dr *dr;
	int ret;

	if (!dev || !drv || !drv->name || !drv->count || !drv->owner)
		return -EINVAL;

	drv->devm_dev = dev;
	drv->devm_managed = true;

	drv->devices = devm_kcalloc(dev, drv->count, sizeof(*drv->devices),
				    GFP_KERNEL);
	if (!drv->devices)
		return -ENOMEM;

	dr = devm_kzalloc(dev, sizeof(*dr), GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	dr->count = drv->count;
	dr->major = drv->major;

	if (drv->major) {
		ret = register_chrdev_region(MKDEV(drv->major, 0), drv->count,
					     drv->name);
		dr->devt = MKDEV(drv->major, 0);
		drv->devt = dr->devt;
	} else {
		ret = alloc_chrdev_region(&drv->devt, 0, drv->count, drv->name);
		dr->devt = drv->devt;
	}
	if (ret)
		return ret;

	ret = devm_add_action(dev, nv_char_devm_chrdev_release, dr);
	if (ret) {
		nv_char_devm_chrdev_release(dr);
		return ret;
	}

	drv->class = class_create(drv->name);
	if (IS_ERR(drv->class)) {
		ret = PTR_ERR(drv->class);
		devm_release_action(dev, nv_char_devm_chrdev_release, dr);
		nv_char_devm_chrdev_release(dr);
		return ret;
	}

	drv->class->devnode = nv_char_devnode;

	ret = devm_add_action(dev, nv_char_devm_class_release, drv);
	if (ret) {
		class_destroy(drv->class);
		drv->class = NULL;
		devm_release_action(dev, nv_char_devm_chrdev_release, dr);
		nv_char_devm_chrdev_release(dr);
		return ret;
	}

	nv_char_debugfs_init();
	return 0;
}
EXPORT_SYMBOL_GPL(nv_char_driver_register_devm);

void nv_char_driver_unregister_devm(struct nv_char_driver *drv)
{
	unsigned int i;

	if (!drv)
		return;

	if (drv->devices) {
		for (i = 0; i < drv->count; i++) {
			if (drv->devices[i])
				nv_char_device_unregister(drv->devices[i]);
		}
		drv->devices = NULL;
	}

	nv_char_debugfs_exit();
	drv->class = NULL;
	drv->devt = 0;
	drv->devm_managed = false;
	drv->devm_dev = NULL;
}
EXPORT_SYMBOL_GPL(nv_char_driver_unregister_devm);

int nv_char_device_register_parent(struct nv_char_driver *drv,
				   struct nv_char_dev *dev, unsigned int index,
				   const char *name,
				   const struct nv_char_ops *ops, void *priv,
				   struct device *parent)
{
	int ret;

	if (!drv || !dev || !name || index >= drv->count)
		return -EINVAL;

	if (dev->registered)
		return -EBUSY;

	dev->driver = drv;
	dev->owner = drv->owner;
	dev->index = index;
	dev->name = name;
	dev->ops = ops;
	dev->priv = priv;
	dev->removing = false;
	dev->devt = MKDEV(MAJOR(drv->devt), MINOR(drv->devt) + index);

	if (!dev->dev_mode && drv->dev_mode)
		dev->dev_mode = drv->dev_mode;

	atomic_set(&dev->opens, 0);
	init_waitqueue_head(&dev->wq);
	init_waitqueue_head(&dev->release_wq);

	cdev_init(&dev->cdev, &nv_char_fops);
	dev->cdev.owner = drv->owner;

	ret = cdev_add(&dev->cdev, dev->devt, 1);
	if (ret)
		return ret;

	dev->device = device_create(drv->class, parent, dev->devt, dev, "%s",
				    name);
	if (IS_ERR(dev->device)) {
		ret = PTR_ERR(dev->device);
		dev->device = NULL;
		cdev_del(&dev->cdev);
		return ret;
	}

	ret = device_create_file(dev->device, &dev_attr_opens);
	if (ret) {
		device_destroy(drv->class, dev->devt);
		dev->device = NULL;
		cdev_del(&dev->cdev);
		return ret;
	}

	nv_char_debugfs_add_device(dev);

	drv->devices[index] = dev;
	dev->registered = true;

	trace_nv_char_register(dev->devt);
	return 0;
}
EXPORT_SYMBOL_GPL(nv_char_device_register_parent);

int nv_char_device_register(struct nv_char_driver *drv,
			    struct nv_char_dev *dev, unsigned int index,
			    const char *name, const struct nv_char_ops *ops,
			    void *priv)
{
	return nv_char_device_register_parent(drv, dev, index, name, ops, priv,
					      NULL);
}
EXPORT_SYMBOL_GPL(nv_char_device_register);

void nv_char_device_unregister(struct nv_char_dev *dev)
{
	long remaining;

	if (!dev || !dev->registered)
		return;

	trace_nv_char_unregister(dev->devt);

	WRITE_ONCE(dev->removing, true);
	smp_mb();

	if (dev->driver && dev->driver->devices)
		dev->driver->devices[dev->index] = NULL;

	nv_char_debugfs_remove_device(dev);

	if (dev->device) {
		device_remove_file(dev->device, &dev_attr_opens);
		device_destroy(dev->driver->class, dev->devt);
		dev->device = NULL;
	}

	remaining = wait_event_timeout(dev->release_wq,
				     atomic_read(&dev->opens) == 0,
				     NV_CHAR_UNREG_TIMEOUT);
	if (remaining == 0)
		pr_warn("nv_char: %s still has %d open fd(s) after unregister\n",
			dev->name, atomic_read(&dev->opens));

	cdev_del(&dev->cdev);
	dev->registered = false;
}
EXPORT_SYMBOL_GPL(nv_char_device_unregister);

int nv_char_misc_register(struct nv_char_misc_dev *misc, const char *name,
			  const struct nv_char_ops *ops, void *priv,
			  struct module *owner)
{
	struct nv_char_dev *dev;
	int ret;

	if (!misc || !name || !owner)
		return -EINVAL;

	if (misc->registered)
		return -EBUSY;

	dev = &misc->dev;
	dev->driver = NULL;
	dev->owner = owner;
	dev->name = name;
	dev->ops = ops;
	dev->priv = priv;
	dev->removing = false;
	dev->index = 0;

	atomic_set(&dev->opens, 0);
	init_waitqueue_head(&dev->wq);
	init_waitqueue_head(&dev->release_wq);

	nv_char_debugfs_init();

	misc->misc.minor = MISC_DYNAMIC_MINOR;
	misc->misc.name = name;
	misc->misc.fops = &nv_char_misc_fops;
	misc->misc.parent = NULL;
	misc->misc.nodename = name;

	ret = misc_register(&misc->misc);
	if (ret)
		goto err_debugfs;

	nv_char_debugfs_add_device(dev);
	misc->registered = true;
	return 0;

err_debugfs:
	nv_char_debugfs_exit();
	return ret;
}
EXPORT_SYMBOL_GPL(nv_char_misc_register);

void nv_char_misc_unregister(struct nv_char_misc_dev *misc)
{
	struct nv_char_dev *dev;
	long remaining;

	if (!misc || !misc->registered)
		return;

	dev = &misc->dev;
	WRITE_ONCE(dev->removing, true);
	smp_mb();

	misc_deregister(&misc->misc);
	nv_char_debugfs_remove_device(dev);

	remaining = wait_event_timeout(dev->release_wq,
				     atomic_read(&dev->opens) == 0,
				     NV_CHAR_UNREG_TIMEOUT);
	if (remaining == 0)
		pr_warn("nv_char: misc %s still has %d open fd(s)\n",
			dev->name, atomic_read(&dev->opens));

	nv_char_debugfs_exit();
	misc->registered = false;
}
EXPORT_SYMBOL_GPL(nv_char_misc_unregister);
