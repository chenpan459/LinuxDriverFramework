// SPDX-License-Identifier: GPL-2.0
/*
 * Example: platform + devm (chrdev region/class auto-released on device unbind)
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "nv_char.h"

#define PLAT_DEV_NAME	"nv_plat"

struct plat_priv {
	char *buf;
	size_t len;
	struct mutex lock;
};

struct plat_ctx {
	struct platform_device *pdev;
	struct nv_char_driver drv;
	struct nv_char_dev dev;
	struct plat_priv priv;
};

static struct plat_ctx plat;

static ssize_t plat_read(struct nv_char_dev *dev, struct file *filp,
			 char __user *buf, size_t count, loff_t *ppos)
{
	struct plat_priv *priv = dev->priv;
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

static ssize_t plat_write(struct nv_char_dev *dev, struct file *filp,
			  const char __user *buf, size_t count, loff_t *ppos)
{
	struct plat_priv *priv = dev->priv;
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

static const struct nv_char_ops plat_ops = {
	.read	= plat_read,
	.write	= plat_write,
};

static int nv_char_plat_probe(struct platform_device *pdev)
{
	int ret;

	plat.pdev = pdev;
	plat.priv.buf = devm_kzalloc(&pdev->dev, PAGE_SIZE, GFP_KERNEL);
	if (!plat.priv.buf)
		return -ENOMEM;
	mutex_init(&plat.priv.lock);

	plat.drv.name = "nv_char_plat";
	plat.drv.owner = THIS_MODULE;
	plat.drv.count = 1;
	plat.drv.dev_mode = 0660;

	ret = nv_char_driver_register_devm(&pdev->dev, &plat.drv);
	if (ret)
		return ret;

	ret = nv_char_device_register_parent(&plat.drv, &plat.dev, 0,
					     PLAT_DEV_NAME, &plat_ops,
					     &plat.priv, &pdev->dev);
	if (ret) {
		nv_char_driver_unregister_devm(&plat.drv);
		return ret;
	}

	dev_info(&pdev->dev, "nv_char_plat: /dev/%s (devm)\n", PLAT_DEV_NAME);
	return 0;
}

static void nv_char_plat_remove(struct platform_device *pdev)
{
	nv_char_device_unregister(&plat.dev);
	nv_char_driver_unregister_devm(&plat.drv);
}

#ifdef CONFIG_OF
static const struct of_device_id nv_char_plat_of_match[] = {
	{ .compatible = "nv,char-plat" },
	{ }
};
MODULE_DEVICE_TABLE(of, nv_char_plat_of_match);
#endif

static struct platform_driver nv_char_plat_driver = {
	.probe	= nv_char_plat_probe,
	.remove	= nv_char_plat_remove,
	.driver = {
		.name = "nv-char-plat",
		.of_match_table = of_match_ptr(nv_char_plat_of_match),
	},
};

static int __init plat_init(void)
{
	int ret;

	ret = platform_driver_register(&nv_char_plat_driver);
	if (ret)
		return ret;

	plat.pdev = platform_device_register_simple("nv-char-plat", -1, NULL, 0);
	if (IS_ERR(plat.pdev)) {
		ret = PTR_ERR(plat.pdev);
		platform_driver_unregister(&nv_char_plat_driver);
		return ret;
	}

	return 0;
}

static void __exit plat_exit(void)
{
	platform_device_unregister(plat.pdev);
	platform_driver_unregister(&nv_char_plat_driver);
}

module_init(plat_init);
module_exit(plat_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("Platform + devm example for nv_char");
MODULE_LICENSE("GPL");
