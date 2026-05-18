/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - SPI device helper layer
 */
#ifndef _NV_SPI_H_
#define _NV_SPI_H_

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/types.h>

struct nv_spi_dev;
struct nv_spi_driver;

/**
 * struct nv_spi_ops - driver callbacks invoked by the framework
 */
struct nv_spi_ops {
	int (*probe)(struct nv_spi_dev *dev, const struct spi_device_id *id);
	void (*remove)(struct nv_spi_dev *dev);
	void (*shutdown)(struct nv_spi_dev *dev);
};

/**
 * struct nv_spi_dev - per-SPI device context (spi_set_drvdata)
 */
struct nv_spi_dev {
	struct spi_device *spi;
	struct nv_spi_driver *driver;
	const struct spi_device_id *id;
	void *priv;

	u32 bus_num;
	u32 chip_select;
	u32 max_speed_hz;
	u8 mode;
	u8 bits_per_word;
};

/**
 * struct nv_spi_driver - SPI driver registration wrapper
 */
struct nv_spi_driver {
	const char *name;
	struct module *owner;
	const struct spi_device_id *id_table;
	const struct of_device_id *of_match_table;
	const struct nv_spi_ops *ops;
	unsigned int flags;

	struct spi_driver spi_driver;
	bool registered;
};

#define NV_SPI_FLAG_SKIP_SPI_SETUP	BIT(0)

int nv_spi_driver_register(struct nv_spi_driver *drv);
void nv_spi_driver_unregister(struct nv_spi_driver *drv);

struct nv_spi_dev *nv_spi_dev_from_spi(struct spi_device *spi);

int nv_spi_setup(struct nv_spi_dev *dev);
int nv_spi_set_speed(struct nv_spi_dev *dev, u32 hz);
int nv_spi_set_mode(struct nv_spi_dev *dev, u8 mode);

int nv_spi_write(struct nv_spi_dev *dev, const void *buf, size_t len);
int nv_spi_read(struct nv_spi_dev *dev, void *buf, size_t len);
int nv_spi_write_then_read(struct nv_spi_dev *dev,
			   const void *txbuf, unsigned n_tx,
			   void *rxbuf, unsigned n_rx);
int nv_spi_xfer(struct nv_spi_dev *dev, const void *txbuf, void *rxbuf,
		size_t len);

static inline struct device *nv_spi_dev_device(struct nv_spi_dev *dev)
{
	return dev && dev->spi ? &dev->spi->dev : NULL;
}

#endif /* _NV_SPI_H_ */
