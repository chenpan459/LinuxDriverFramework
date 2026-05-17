/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - character device helper layer (cdev)
 *
 * Per device: cdev_init -> cdev_add -> device_create
 * Unregister:  removing flag -> device_destroy -> wait opens -> cdev_del
 */
#ifndef _NV_CHAR_H_
#define _NV_CHAR_H_

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>

struct dentry;
#include <linux/kdev_t.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/wait.h>

struct nv_char_dev;
struct nv_char_driver;
struct nv_char_misc_dev;

/**
 * struct nv_char_ioctl_entry - ioctl dispatch table item
 * @cmd:      ioctl command (_IO / _IOR / _IOW / _IOWR)
 * @handler:  handler(dev, filp, arg), return negative errno
 * @capable:  0 = no check; else require capable(capable) (e.g. CAP_SYS_ADMIN)
 */
struct nv_char_ioctl_entry {
	unsigned int cmd;
	int (*handler)(struct nv_char_dev *dev, struct file *filp,
		       unsigned long arg);
	int capable;
};

/**
 * struct nv_char_ops - per-device file operation callbacks (all optional)
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
	ssize_t (*read_iter)(struct nv_char_dev *dev, struct kiocb *iocb,
			     struct iov_iter *iter);
	ssize_t (*write_iter)(struct nv_char_dev *dev, struct kiocb *iocb,
			      struct iov_iter *iter);
	long (*unlocked_ioctl)(struct nv_char_dev *dev, struct file *filp,
			       unsigned int cmd, unsigned long arg);
	long (*compat_ioctl)(struct nv_char_dev *dev, struct file *filp,
			     unsigned int cmd, unsigned long arg);
	__poll_t (*poll)(struct nv_char_dev *dev, struct file *filp,
			 poll_table *wait);
	loff_t (*llseek)(struct nv_char_dev *dev, struct file *filp,
			 loff_t offset, int whence);
	int (*mmap)(struct nv_char_dev *dev, struct file *filp,
		    struct vm_area_struct *vma);
	int (*fsync)(struct nv_char_dev *dev, struct file *filp,
		     loff_t start, loff_t end, int datasync);
};

/**
 * struct nv_char_dev - one character device instance
 * @wq:         default wait queue for poll / wake helpers
 * @opens:      number of active file descriptors
 * @removing:   set during unregister; new opens get -ENODEV
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
	bool removing;

	atomic_t opens;
	wait_queue_head_t wq;
	wait_queue_head_t release_wq;

	umode_t dev_mode;
	struct dentry *debugfs_dir;

	/* Used when not bound to nv_char_driver (miscdevice path) */
	struct module *owner;
};

/**
 * struct nv_char_misc_dev - single miscdevice instance (MISC_DYNAMIC_MINOR)
 */
struct nv_char_misc_dev {
	struct nv_char_dev dev;
	struct miscdevice misc;
	bool registered;
};

/**
 * struct nv_char_driver - driver-wide state
 * @major: 0 = dynamic (alloc_chrdev_region); else static major number
 */
struct nv_char_driver {
	const char *name;
	struct module *owner;
	unsigned int count;
	unsigned int major;

	dev_t devt;
	struct class *class;
	struct nv_char_dev **devices;
	umode_t dev_mode;

	/* devm: resources tied to struct device lifetime */
	struct device *devm_dev;
	bool devm_managed;
};

int nv_char_driver_register(struct nv_char_driver *drv);
void nv_char_driver_unregister(struct nv_char_driver *drv);

int nv_char_driver_register_devm(struct device *dev, struct nv_char_driver *drv);
void nv_char_driver_unregister_devm(struct nv_char_driver *drv);

int nv_char_device_register(struct nv_char_driver *drv,
			    struct nv_char_dev *dev, unsigned int index,
			    const char *name, const struct nv_char_ops *ops,
			    void *priv);
int nv_char_device_register_parent(struct nv_char_driver *drv,
				   struct nv_char_dev *dev, unsigned int index,
				   const char *name,
				   const struct nv_char_ops *ops, void *priv,
				   struct device *parent);
void nv_char_device_unregister(struct nv_char_dev *dev);

int nv_char_misc_register(struct nv_char_misc_dev *misc, const char *name,
			  const struct nv_char_ops *ops, void *priv,
			  struct module *owner);
void nv_char_misc_unregister(struct nv_char_misc_dev *misc);

void nv_char_debugfs_init(void);
void nv_char_debugfs_exit(void);

long nv_char_ioctl_dispatch(struct nv_char_dev *dev, struct file *filp,
			    unsigned int cmd, unsigned long arg,
			    const struct nv_char_ioctl_entry *table,
			    unsigned int nent);

void nv_char_wake(struct nv_char_dev *dev);

static inline unsigned int nv_char_dev_major(struct nv_char_dev *dev)
{
	return dev ? MAJOR(dev->devt) : 0;
}

static inline unsigned int nv_char_dev_minor(struct nv_char_dev *dev)
{
	return dev ? MINOR(dev->devt) : 0;
}

static inline int nv_char_open_count(struct nv_char_dev *dev)
{
	return dev ? atomic_read(&dev->opens) : 0;
}

#endif /* _NV_CHAR_H_ */
