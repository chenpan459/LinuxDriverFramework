/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - character device helper layer
 */
#ifndef _NV_CHAR_H_
#define _NV_CHAR_H_

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/types.h>

struct nv_char_dev;
struct nv_char_driver;

/**
 * struct nv_char_ops - per-device file operation callbacks
 *
 * All members are optional; omitted handlers get sensible defaults
 * (e.g. -EINVAL for ioctl, -ENODEV for read/write when not provided).
 */
struct nv_char_ops {
	int (*open)(struct nv_char_dev *dev, struct inode *inode,
		    struct file *filp);
	int (*release)(struct nv_char_dev *dev, struct inode *inode,
		       struct file *filp);
	ssize_t (*read)(struct nv_char_dev *dev, struct file *filp,
			char __user *buf, size_t count, loff_t *ppos);
	ssize_t (*write)(struct nv_char_dev *dev, struct file *filp,
			 const char __user *buf, size_t count, loff_t *ppos);
	long (*unlocked_ioctl)(struct nv_char_dev *dev, struct file *filp,
			       unsigned int cmd, unsigned long arg);
	long (*compat_ioctl)(struct nv_char_dev *dev, struct file *filp,
			     unsigned int cmd, unsigned long arg);
	__poll_t (*poll)(struct nv_char_dev *dev, struct file *filp,
			 poll_table *wait);
	loff_t (*llseek)(struct nv_char_dev *dev, struct file *filp,
			 loff_t offset, int whence);
};

/**
 * struct nv_char_dev - one character device instance
 */
struct nv_char_dev {
	struct nv_char_driver *driver;
	unsigned int index;
	const char *name;
	const struct nv_char_ops *ops;
	void *priv;

	dev_t devt;
	struct cdev cdev;
	struct device *device;
	bool registered;
};

/**
 * struct nv_char_driver - driver-wide state (major, class, device count)
 */
struct nv_char_driver {
	const char *name;
	struct module *owner;
	unsigned int count;

	dev_t devt;
	struct class *class;
	struct nv_char_dev **devices;
};

int nv_char_driver_register(struct nv_char_driver *drv);
void nv_char_driver_unregister(struct nv_char_driver *drv);

int nv_char_device_register(struct nv_char_driver *drv,
			     struct nv_char_dev *dev, unsigned int index,
			     const char *name, const struct nv_char_ops *ops,
			     void *priv);
void nv_char_device_unregister(struct nv_char_dev *dev);

#endif /* _NV_CHAR_H_ */
