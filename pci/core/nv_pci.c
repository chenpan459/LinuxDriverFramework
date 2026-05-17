// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - PCI/PCIe helper implementation
 */

#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>

#include "nv_pci.h"

#ifndef PCI_IRQ_INTX
#define PCI_IRQ_INTX PCI_IRQ_LEGACY
#endif

static struct nv_pci_driver *nv_pci_drv_from_pdev(struct pci_dev *pdev)
{
	if (!pdev || !pdev->driver)
		return NULL;

	return container_of(pdev->driver, struct nv_pci_driver, pci_driver);
}

static struct nv_pci_driver *nv_pci_drv_from_id(const struct pci_device_id *id)
{
	if (!id || !id->driver_data)
		return NULL;

	return (struct nv_pci_driver *)(unsigned long)id->driver_data;
}

struct nv_pci_dev *nv_pci_dev_from_pdev(struct pci_dev *pdev)
{
	return pdev ? pci_get_drvdata(pdev) : NULL;
}
EXPORT_SYMBOL_GPL(nv_pci_dev_from_pdev);

int nv_pci_enable(struct nv_pci_dev *dev)
{
	struct pci_dev *pdev;
	int ret;

	if (!dev || !dev->pdev)
		return -EINVAL;

	pdev = dev->pdev;
	if (dev->enabled)
		return 0;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	if (!(dev->driver->flags & NV_PCI_FLAG_SKIP_MWI)) {
		ret = pcim_set_mwi(pdev);
		if (ret)
			return ret;
	}

	if (!(dev->driver->flags & NV_PCI_FLAG_SKIP_SET_MASTER))
		pci_set_master(pdev);

	dev->enabled = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_pci_enable);

void nv_pci_disable(struct nv_pci_dev *dev)
{
	if (!dev || !dev->pdev || !dev->enabled)
		return;

	pci_clear_master(dev->pdev);
	dev->enabled = false;
}
EXPORT_SYMBOL_GPL(nv_pci_disable);

int nv_pci_iomap_bar(struct nv_pci_dev *dev, int bar, unsigned long maxlen)
{
	if (!dev || !dev->pdev || bar < 0 || bar >= NV_PCI_MAX_BARS)
		return -EINVAL;

	if (dev->bar[bar])
		return 0;

	if (!pci_resource_len(dev->pdev, bar))
		return -ENOENT;

	dev->bar[bar] = pcim_iomap(dev->pdev, bar, maxlen);
	if (!dev->bar[bar])
		return -ENOMEM;

	return 0;
}
EXPORT_SYMBOL_GPL(nv_pci_iomap_bar);

void nv_pci_iounmap_bar(struct nv_pci_dev *dev, int bar)
{
	if (!dev || bar < 0 || bar >= NV_PCI_MAX_BARS)
		return;

	dev->bar[bar] = NULL;
}
EXPORT_SYMBOL_GPL(nv_pci_iounmap_bar);

resource_size_t nv_pci_bar_start(struct nv_pci_dev *dev, int bar)
{
	if (!dev || !dev->pdev || bar < 0 || bar >= NV_PCI_MAX_BARS)
		return 0;

	return pci_resource_start(dev->pdev, bar);
}
EXPORT_SYMBOL_GPL(nv_pci_bar_start);

resource_size_t nv_pci_bar_len(struct nv_pci_dev *dev, int bar)
{
	if (!dev || !dev->pdev || bar < 0 || bar >= NV_PCI_MAX_BARS)
		return 0;

	return pci_resource_len(dev->pdev, bar);
}
EXPORT_SYMBOL_GPL(nv_pci_bar_len);

int nv_pci_request_irq(struct nv_pci_dev *dev, irq_handler_t handler,
		       const char *name, unsigned long flags, void *data)
{
	struct pci_dev *pdev;
	int ret;

	if (!dev || !dev->pdev || !handler)
		return -EINVAL;

	pdev = dev->pdev;
	if (dev->irq > 0)
		return 0;

	ret = pci_alloc_irq_vectors(pdev, 1, 1,
				    PCI_IRQ_MSI | PCI_IRQ_INTX);
	if (ret < 0)
		return ret;

	dev->num_irq_vectors = ret;
	dev->irq = pci_irq_vector(pdev, 0);
	if (dev->irq < 0) {
		pci_free_irq_vectors(pdev);
		dev->num_irq_vectors = 0;
		return -ENOENT;
	}

	ret = devm_request_irq(&pdev->dev, dev->irq, handler, flags,
			       name ? name : dev->driver->name, data);
	if (ret) {
		pci_free_irq_vectors(pdev);
		dev->irq = -1;
		dev->num_irq_vectors = 0;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(nv_pci_request_irq);

void nv_pci_free_irq(struct nv_pci_dev *dev)
{
	if (!dev || !dev->pdev)
		return;

	if (dev->irq > 0)
		devm_free_irq(&dev->pdev->dev, dev->irq, dev);

	if (dev->num_irq_vectors)
		pci_free_irq_vectors(dev->pdev);

	dev->irq = -1;
	dev->num_irq_vectors = 0;
}
EXPORT_SYMBOL_GPL(nv_pci_free_irq);

static int nv_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct nv_pci_driver *drv;
	struct nv_pci_dev *dev;
	int ret;

	drv = nv_pci_drv_from_id(id);
	if (!drv)
		drv = nv_pci_drv_from_pdev(pdev);
	if (!drv || !drv->ops || !drv->ops->probe)
		return -ENODEV;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdev = pdev;
	dev->driver = drv;
	dev->id = id;
	dev->irq = -1;
	pci_set_drvdata(pdev, dev);

	ret = nv_pci_enable(dev);
	if (ret)
		return ret;

	ret = drv->ops->probe(dev, id);
	if (ret) {
		nv_pci_disable(dev);
		return ret;
	}

	dev_info(&pdev->dev, "nv_pci: %s bound (%04x:%04x)\n",
		 drv->name, pdev->vendor, pdev->device);
	return 0;
}

static void nv_pci_remove(struct pci_dev *pdev)
{
	struct nv_pci_driver *drv = nv_pci_drv_from_pdev(pdev);
	struct nv_pci_dev *dev = nv_pci_dev_from_pdev(pdev);

	if (!dev)
		return;

	nv_pci_free_irq(dev);

	if (drv && drv->ops && drv->ops->remove)
		drv->ops->remove(dev);

	nv_pci_disable(dev);
	pci_set_drvdata(pdev, NULL);
}

static int nv_pci_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct nv_pci_driver *drv = nv_pci_drv_from_pdev(pdev);
	struct nv_pci_dev *dev = nv_pci_dev_from_pdev(pdev);

	if (drv && drv->ops && drv->ops->suspend && dev)
		return drv->ops->suspend(dev, state);

	return 0;
}

static int nv_pci_resume(struct pci_dev *pdev)
{
	struct nv_pci_driver *drv = nv_pci_drv_from_pdev(pdev);
	struct nv_pci_dev *dev = nv_pci_dev_from_pdev(pdev);

	if (drv && drv->ops && drv->ops->resume && dev)
		return drv->ops->resume(dev);

	return 0;
}

static void nv_pci_shutdown(struct pci_dev *pdev)
{
	struct nv_pci_driver *drv = nv_pci_drv_from_pdev(pdev);
	struct nv_pci_dev *dev = nv_pci_dev_from_pdev(pdev);

	if (drv && drv->ops && drv->ops->shutdown && dev)
		drv->ops->shutdown(dev);
}

int nv_pci_driver_register(struct nv_pci_driver *drv)
{
	int ret;

	if (!drv || !drv->name || !drv->owner || !drv->id_table || !drv->ops)
		return -EINVAL;

	if (drv->registered)
		return -EBUSY;

	drv->pci_driver.name = drv->name;
	drv->pci_driver.id_table = drv->id_table;
	drv->pci_driver.probe = nv_pci_probe;
	drv->pci_driver.remove = nv_pci_remove;
	drv->pci_driver.suspend = nv_pci_suspend;
	drv->pci_driver.resume = nv_pci_resume;
	drv->pci_driver.shutdown = nv_pci_shutdown;

	ret = pci_register_driver(&drv->pci_driver);
	if (ret)
		return ret;

	drv->registered = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_pci_driver_register);

void nv_pci_driver_unregister(struct nv_pci_driver *drv)
{
	if (!drv || !drv->registered)
		return;

	pci_unregister_driver(&drv->pci_driver);
	drv->registered = false;
}
EXPORT_SYMBOL_GPL(nv_pci_driver_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("PCI/PCIe device framework");
MODULE_LICENSE("GPL");
