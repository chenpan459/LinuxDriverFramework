/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - PCI/PCIe device helper layer
 */
#ifndef _NV_PCI_H_
#define _NV_PCI_H_

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/types.h>

#define NV_PCI_MAX_BARS	6

struct nv_pci_dev;
struct nv_pci_driver;

/**
 * struct nv_pci_ops - driver callbacks invoked by the framework
 */
struct nv_pci_ops {
	int (*probe)(struct nv_pci_dev *dev, const struct pci_device_id *id);
	void (*remove)(struct nv_pci_dev *dev);
	int (*suspend)(struct nv_pci_dev *dev, pm_message_t state);
	int (*resume)(struct nv_pci_dev *dev);
	void (*shutdown)(struct nv_pci_dev *dev);
};

/**
 * struct nv_pci_dev - per-PCI function context (stored in pci_set_drvdata)
 */
struct nv_pci_dev {
	struct pci_dev *pdev;
	struct nv_pci_driver *driver;
	const struct pci_device_id *id;
	void *priv;

	void __iomem *bar[NV_PCI_MAX_BARS];
	int irq;
	unsigned int num_irq_vectors;
	bool enabled;
};

/**
 * struct nv_pci_driver - PCI driver registration wrapper
 * @flags: NV_PCI_FLAG_* for enable path
 */
struct nv_pci_driver {
	const char *name;
	struct module *owner;
	const struct pci_device_id *id_table;
	const struct nv_pci_ops *ops;
	unsigned int flags;

	struct pci_driver pci_driver;
	bool registered;
};

#define NV_PCI_FLAG_SKIP_MWI		BIT(0)
#define NV_PCI_FLAG_SKIP_SET_MASTER	BIT(1)

int nv_pci_driver_register(struct nv_pci_driver *drv);
void nv_pci_driver_unregister(struct nv_pci_driver *drv);

struct nv_pci_dev *nv_pci_dev_from_pdev(struct pci_dev *pdev);

int nv_pci_enable(struct nv_pci_dev *dev);
void nv_pci_disable(struct nv_pci_dev *dev);

int nv_pci_iomap_bar(struct nv_pci_dev *dev, int bar, unsigned long maxlen);
void nv_pci_iounmap_bar(struct nv_pci_dev *dev, int bar);
resource_size_t nv_pci_bar_start(struct nv_pci_dev *dev, int bar);
resource_size_t nv_pci_bar_len(struct nv_pci_dev *dev, int bar);

int nv_pci_request_irq(struct nv_pci_dev *dev, irq_handler_t handler,
		       const char *name, unsigned long flags, void *data);
void nv_pci_free_irq(struct nv_pci_dev *dev);

static inline struct device *nv_pci_dev_device(struct nv_pci_dev *dev)
{
	return dev && dev->pdev ? &dev->pdev->dev : NULL;
}

#endif /* _NV_PCI_H_ */
