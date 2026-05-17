// SPDX-License-Identifier: GPL-2.0
/*
 * Example: PCI driver using nv_pci framework
 *
 * Matches QEMU "edu" device (vendor 0x1234 device 0x11e9) or load without
 * hardware (driver registers, probe runs when device is present).
 *
 * QEMU:
 *   -device edu
 *
 * Load:  sudo insmod nv_pci_demo.ko
 * Unload: sudo rmmod nv_pci_demo
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#include "nv_pci.h"

#define DRV_NAME	"nv_pci_demo"

/* QEMU edu device (see qemu/hw/misc/edu.c) */
#define NV_PCI_DEMO_VENDOR	0x1234
#define NV_PCI_DEMO_DEVICE	0x11e9

struct demo_priv {
	void __iomem *mmio;
	resource_size_t mmio_len;
};

static int demo_probe(struct nv_pci_dev *dev, const struct pci_device_id *id)
{
	struct demo_priv *priv;
	int ret;

	priv = devm_kzalloc(nv_pci_dev_device(dev), sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	ret = nv_pci_iomap_bar(dev, 0, 0);
	if (ret)
		return ret;

	priv->mmio = dev->bar[0];
	priv->mmio_len = nv_pci_bar_len(dev, 0);
	dev->priv = priv;

	{
		resource_size_t bar0 = nv_pci_bar_start(dev, 0);

		pci_info(dev->pdev,
			 "nv_pci_demo: BAR0 %pa len %pa (edu or compatible)\n",
			 &bar0, &priv->mmio_len);
	}
	return 0;
}

static void demo_remove(struct nv_pci_dev *dev)
{
	struct demo_priv *priv = dev->priv;

	if (priv)
		nv_pci_iounmap_bar(dev, 0);
	dev->priv = NULL;
}

static const struct nv_pci_ops demo_ops = {
	.probe	= demo_probe,
	.remove	= demo_remove,
};

static struct nv_pci_driver demo_driver;

static const struct pci_device_id demo_ids[] = {
	{
		PCI_DEVICE(NV_PCI_DEMO_VENDOR, NV_PCI_DEMO_DEVICE),
		.driver_data = (kernel_ulong_t)&demo_driver,
	},
	{ }
};
MODULE_DEVICE_TABLE(pci, demo_ids);

static int __init demo_init(void)
{
	demo_driver.name = DRV_NAME;
	demo_driver.owner = THIS_MODULE;
	demo_driver.id_table = demo_ids;
	demo_driver.ops = &demo_ops;

	return nv_pci_driver_register(&demo_driver);
}

static void __exit demo_exit(void)
{
	nv_pci_driver_unregister(&demo_driver);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("PCI demo driver (QEMU edu)");
MODULE_LICENSE("GPL");
