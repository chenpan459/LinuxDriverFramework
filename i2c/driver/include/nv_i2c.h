/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - I2C device helper layer
 */
#ifndef _NV_I2C_H_
#define _NV_I2C_H_

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/types.h>

struct nv_i2c_dev;
struct nv_i2c_driver;

/**
 * struct nv_i2c_ops - driver callbacks invoked by the framework
 */
struct nv_i2c_ops {
	int (*probe)(struct nv_i2c_dev *dev, const struct i2c_device_id *id);
	void (*remove)(struct nv_i2c_dev *dev);
	void (*shutdown)(struct nv_i2c_dev *dev);
	void (*alert)(struct nv_i2c_dev *dev, enum i2c_alert_protocol protocol,
		      unsigned int data);
};

/**
 * struct nv_i2c_dev - per-I2C client context (i2c_set_clientdata)
 */
struct nv_i2c_dev {
	struct i2c_client *client;
	struct nv_i2c_driver *driver;
	const struct i2c_device_id *id;
	void *priv;

	u32 adapter_nr;
	u16 addr;
};

/**
 * struct nv_i2c_driver - I2C driver registration wrapper
 */
struct nv_i2c_driver {
	const char *name;
	struct module *owner;
	const struct i2c_device_id *id_table;
	const struct nv_i2c_ops *ops;
	unsigned int flags;

	struct i2c_driver i2c_driver;
	bool registered;
};

#define NV_I2C_FLAG_USE_10BIT_ADDR	BIT(0)

int nv_i2c_driver_register(struct nv_i2c_driver *drv);
void nv_i2c_driver_unregister(struct nv_i2c_driver *drv);

struct nv_i2c_dev *nv_i2c_dev_from_client(struct i2c_client *client);

int nv_i2c_read(struct nv_i2c_dev *dev, u8 *buf, int len);
int nv_i2c_write(struct nv_i2c_dev *dev, const u8 *buf, int len);
int nv_i2c_smbus_read_byte(struct nv_i2c_dev *dev);
int nv_i2c_smbus_write_byte(struct nv_i2c_dev *dev, u8 value);
int nv_i2c_smbus_read_byte_data(struct nv_i2c_dev *dev, u8 command);
int nv_i2c_smbus_write_byte_data(struct nv_i2c_dev *dev, u8 command, u8 value);
int nv_i2c_smbus_read_word_data(struct nv_i2c_dev *dev, u8 command);
int nv_i2c_smbus_write_word_data(struct nv_i2c_dev *dev, u8 command, u16 value);

static inline struct device *nv_i2c_dev_device(struct nv_i2c_dev *dev)
{
	return dev && dev->client ? &dev->client->dev : NULL;
}

#endif /* _NV_I2C_H_ */
