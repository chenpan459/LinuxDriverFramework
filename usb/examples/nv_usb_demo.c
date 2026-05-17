// SPDX-License-Identifier: GPL-2.0
/*
 * Example: USB interface driver using nv_usb framework
 *
 * Matches the classic usb-skeleton test IDs (NetChip 0525:a4a7) when present.
 * Without a matching device the module loads but probe is not called.
 *
 * Test with dummy HCD + gadget (see usb/README.md).
 *
 * Load:   sudo insmod nv_usb_demo.ko
 * Unload: sudo rmmod nv_usb_demo
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>

#include "nv_usb.h"

#define DRV_NAME	"nv_usb_demo"

/* Same placeholder as kernel usb-skeleton sample */
#define NV_USB_DEMO_VENDOR	0x0525
#define NV_USB_DEMO_PRODUCT	0xa4a7

struct demo_priv {
	struct nv_usb_endpoints eps;
};

static int demo_probe(struct nv_usb_dev *dev, const struct usb_device_id *id)
{
	struct demo_priv *priv;
	int ret;

	priv = devm_kzalloc(nv_usb_dev_device(dev), sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	ret = nv_usb_find_endpoints(dev);
	if (ret)
		return ret;

	priv->eps = dev->eps;
	dev->priv = priv;

	dev_info(nv_usb_dev_device(dev),
		 "nv_usb_demo: bulk in %s out %s (mps in %u out %u)\n",
		 dev->eps.bulk_in_addr ? "yes" : "no",
		 dev->eps.bulk_out_addr ? "yes" : "no",
		 dev->eps.bulk_in_mps, dev->eps.bulk_out_mps);
	return 0;
}

static void demo_disconnect(struct nv_usb_dev *dev)
{
	dev->priv = NULL;
}

static const struct nv_usb_ops demo_ops = {
	.probe		= demo_probe,
	.disconnect	= demo_disconnect,
};

static struct nv_usb_driver demo_driver;

static const struct usb_device_id demo_ids[] = {
	{
		USB_DEVICE(NV_USB_DEMO_VENDOR, NV_USB_DEMO_PRODUCT),
		.driver_info = (kernel_ulong_t)&demo_driver,
	},
	{ }
};
MODULE_DEVICE_TABLE(usb, demo_ids);

static int __init demo_init(void)
{
	demo_driver.name = DRV_NAME;
	demo_driver.owner = THIS_MODULE;
	demo_driver.id_table = demo_ids;
	demo_driver.ops = &demo_ops;

	return nv_usb_driver_register(&demo_driver);
}

static void __exit demo_exit(void)
{
	nv_usb_driver_unregister(&demo_driver);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("USB demo driver (usb-skeleton IDs)");
MODULE_LICENSE("GPL");
