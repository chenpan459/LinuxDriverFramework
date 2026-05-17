// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - SPI device helper implementation
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>

#include "nv_spi.h"

static struct nv_spi_driver *nv_spi_drv_from_spi(struct spi_device *spi)
{
	if (!spi || !spi->dev.driver)
		return NULL;

	return container_of(to_spi_driver(spi->dev.driver),
			    struct nv_spi_driver, spi_driver);
}

static struct nv_spi_driver *nv_spi_drv_from_id(const struct spi_device_id *id)
{
	if (!id || !id->driver_data)
		return NULL;

	return (struct nv_spi_driver *)(unsigned long)id->driver_data;
}

struct nv_spi_dev *nv_spi_dev_from_spi(struct spi_device *spi)
{
	return spi ? spi_get_drvdata(spi) : NULL;
}
EXPORT_SYMBOL_GPL(nv_spi_dev_from_spi);

int nv_spi_setup(struct nv_spi_dev *dev)
{
	if (!dev || !dev->spi)
		return -EINVAL;

	return spi_setup(dev->spi);
}
EXPORT_SYMBOL_GPL(nv_spi_setup);

int nv_spi_set_speed(struct nv_spi_dev *dev, u32 hz)
{
	if (!dev || !dev->spi || !hz)
		return -EINVAL;

	dev->spi->max_speed_hz = hz;
	dev->max_speed_hz = hz;
	return spi_setup(dev->spi);
}
EXPORT_SYMBOL_GPL(nv_spi_set_speed);

int nv_spi_set_mode(struct nv_spi_dev *dev, u8 mode)
{
	if (!dev || !dev->spi)
		return -EINVAL;

	dev->spi->mode = mode;
	dev->mode = mode;
	return spi_setup(dev->spi);
}
EXPORT_SYMBOL_GPL(nv_spi_set_mode);

int nv_spi_write(struct nv_spi_dev *dev, const void *buf, size_t len)
{
	if (!dev || !dev->spi || !buf || !len)
		return -EINVAL;

	return spi_write(dev->spi, buf, len);
}
EXPORT_SYMBOL_GPL(nv_spi_write);

int nv_spi_read(struct nv_spi_dev *dev, void *buf, size_t len)
{
	if (!dev || !dev->spi || !buf || !len)
		return -EINVAL;

	return spi_read(dev->spi, buf, len);
}
EXPORT_SYMBOL_GPL(nv_spi_read);

int nv_spi_write_then_read(struct nv_spi_dev *dev,
			   const void *txbuf, unsigned n_tx,
			   void *rxbuf, unsigned n_rx)
{
	if (!dev || !dev->spi)
		return -EINVAL;

	return spi_write_then_read(dev->spi, txbuf, n_tx, rxbuf, n_rx);
}
EXPORT_SYMBOL_GPL(nv_spi_write_then_read);

int nv_spi_xfer(struct nv_spi_dev *dev, const void *txbuf, void *rxbuf,
		size_t len)
{
	struct spi_transfer xfer = {
		.tx_buf = txbuf,
		.rx_buf = rxbuf,
		.len = len,
	};
	struct spi_message msg;

	if (!dev || !dev->spi || !len)
		return -EINVAL;
	if (!txbuf && !rxbuf)
		return -EINVAL;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);
	return spi_sync(dev->spi, &msg);
}
EXPORT_SYMBOL_GPL(nv_spi_xfer);

static int nv_spi_probe(struct spi_device *spi)
{
	struct nv_spi_driver *drv;
	struct nv_spi_dev *dev;
	const struct spi_device_id *id;
	int ret;

	if (!spi)
		return -EINVAL;

	id = spi_get_device_id(spi);
	drv = nv_spi_drv_from_id(id);
	if (!drv)
		drv = nv_spi_drv_from_spi(spi);
	if (!drv || !drv->ops || !drv->ops->probe)
		return -ENODEV;

	dev = devm_kzalloc(&spi->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->spi = spi;
	dev->driver = drv;
	dev->id = id;
	dev->bus_num = spi->controller->bus_num;
	dev->chip_select = spi_get_chipselect(spi, 0);
	dev->max_speed_hz = spi->max_speed_hz;
	dev->mode = spi->mode;
	dev->bits_per_word = spi->bits_per_word;
	spi_set_drvdata(spi, dev);

	if (!(drv->flags & NV_SPI_FLAG_SKIP_SPI_SETUP)) {
		ret = nv_spi_setup(dev);
		if (ret)
			return ret;
	}

	ret = drv->ops->probe(dev, id);
	if (ret) {
		spi_set_drvdata(spi, NULL);
		return ret;
	}

	dev_info(&spi->dev, "nv_spi: %s bound (spi%u.%u %u Hz mode %u)\n",
		 drv->name, dev->bus_num, dev->chip_select,
		 dev->max_speed_hz, dev->mode);
	return 0;
}

static void nv_spi_remove(struct spi_device *spi)
{
	struct nv_spi_driver *drv = nv_spi_drv_from_spi(spi);
	struct nv_spi_dev *dev = nv_spi_dev_from_spi(spi);

	if (!dev)
		return;

	if (drv && drv->ops && drv->ops->remove)
		drv->ops->remove(dev);

	spi_set_drvdata(spi, NULL);
}

static void nv_spi_shutdown(struct spi_device *spi)
{
	struct nv_spi_driver *drv = nv_spi_drv_from_spi(spi);
	struct nv_spi_dev *dev = nv_spi_dev_from_spi(spi);

	if (drv && drv->ops && drv->ops->shutdown && dev)
		drv->ops->shutdown(dev);
}

int nv_spi_driver_register(struct nv_spi_driver *drv)
{
	int ret;

	if (!drv || !drv->name || !drv->owner || !drv->ops)
		return -EINVAL;
	if (!drv->id_table && !drv->of_match_table)
		return -EINVAL;

	if (drv->registered)
		return -EBUSY;

	drv->spi_driver.driver.name = drv->name;
	drv->spi_driver.driver.owner = drv->owner;
	drv->spi_driver.probe = nv_spi_probe;
	drv->spi_driver.remove = nv_spi_remove;
	drv->spi_driver.shutdown = nv_spi_shutdown;
	drv->spi_driver.id_table = drv->id_table;

	if (drv->of_match_table)
		drv->spi_driver.driver.of_match_table = drv->of_match_table;

	ret = spi_register_driver(&drv->spi_driver);
	if (ret)
		return ret;

	drv->registered = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_spi_driver_register);

void nv_spi_driver_unregister(struct nv_spi_driver *drv)
{
	if (!drv || !drv->registered)
		return;

	spi_unregister_driver(&drv->spi_driver);
	drv->registered = false;
}
EXPORT_SYMBOL_GPL(nv_spi_driver_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("SPI device framework");
MODULE_LICENSE("GPL");
