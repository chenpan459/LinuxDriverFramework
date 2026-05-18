// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - I2C device helper implementation
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>

#include "nv_i2c.h"

static struct nv_i2c_driver *nv_i2c_drv_from_client(struct i2c_client *client)
{
	if (!client || !client->dev.driver)
		return NULL;

	return container_of(to_i2c_driver(client->dev.driver),
			    struct nv_i2c_driver, i2c_driver);
}

static struct nv_i2c_driver *nv_i2c_drv_from_id(const struct i2c_device_id *id)
{
	if (!id || !id->driver_data)
		return NULL;

	return (struct nv_i2c_driver *)(unsigned long)id->driver_data;
}

struct nv_i2c_dev *nv_i2c_dev_from_client(struct i2c_client *client)
{
	return client ? i2c_get_clientdata(client) : NULL;
}
EXPORT_SYMBOL_GPL(nv_i2c_dev_from_client);

int nv_i2c_read(struct nv_i2c_dev *dev, u8 *buf, int len)
{
	if (!dev || !dev->client || !buf || len <= 0)
		return -EINVAL;

	return i2c_master_recv(dev->client, buf, len);
}
EXPORT_SYMBOL_GPL(nv_i2c_read);

int nv_i2c_write(struct nv_i2c_dev *dev, const u8 *buf, int len)
{
	if (!dev || !dev->client || !buf || len <= 0)
		return -EINVAL;

	return i2c_master_send(dev->client, buf, len);
}
EXPORT_SYMBOL_GPL(nv_i2c_write);

int nv_i2c_smbus_read_byte(struct nv_i2c_dev *dev)
{
	if (!dev || !dev->client)
		return -EINVAL;

	return i2c_smbus_read_byte(dev->client);
}
EXPORT_SYMBOL_GPL(nv_i2c_smbus_read_byte);

int nv_i2c_smbus_write_byte(struct nv_i2c_dev *dev, u8 value)
{
	if (!dev || !dev->client)
		return -EINVAL;

	return i2c_smbus_write_byte(dev->client, value);
}
EXPORT_SYMBOL_GPL(nv_i2c_smbus_write_byte);

int nv_i2c_smbus_read_byte_data(struct nv_i2c_dev *dev, u8 command)
{
	if (!dev || !dev->client)
		return -EINVAL;

	return i2c_smbus_read_byte_data(dev->client, command);
}
EXPORT_SYMBOL_GPL(nv_i2c_smbus_read_byte_data);

int nv_i2c_smbus_write_byte_data(struct nv_i2c_dev *dev, u8 command, u8 value)
{
	if (!dev || !dev->client)
		return -EINVAL;

	return i2c_smbus_write_byte_data(dev->client, command, value);
}
EXPORT_SYMBOL_GPL(nv_i2c_smbus_write_byte_data);

int nv_i2c_smbus_read_word_data(struct nv_i2c_dev *dev, u8 command)
{
	if (!dev || !dev->client)
		return -EINVAL;

	return i2c_smbus_read_word_data(dev->client, command);
}
EXPORT_SYMBOL_GPL(nv_i2c_smbus_read_word_data);

int nv_i2c_smbus_write_word_data(struct nv_i2c_dev *dev, u8 command, u16 value)
{
	if (!dev || !dev->client)
		return -EINVAL;

	return i2c_smbus_write_word_data(dev->client, command, value);
}
EXPORT_SYMBOL_GPL(nv_i2c_smbus_write_word_data);

static int nv_i2c_probe(struct i2c_client *client)
{
	struct nv_i2c_driver *drv;
	struct nv_i2c_dev *dev;
	const struct i2c_device_id *id;
	int ret;

	if (!client)
		return -EINVAL;

	id = i2c_client_get_device_id(client);
	drv = nv_i2c_drv_from_id(id);
	if (!drv)
		drv = nv_i2c_drv_from_client(client);
	if (!drv || !drv->ops || !drv->ops->probe)
		return -ENODEV;

	dev = devm_kzalloc(&client->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->client = client;
	dev->driver = drv;
	dev->id = id;
	dev->adapter_nr = client->adapter->nr;
	dev->addr = client->addr;
	i2c_set_clientdata(client, dev);

	ret = drv->ops->probe(dev, id);
	if (ret) {
		i2c_set_clientdata(client, NULL);
		return ret;
	}

	dev_info(&client->dev, "nv_i2c: %s bound (adapter %u addr 0x%02x)\n",
		 drv->name, dev->adapter_nr, dev->addr);
	return 0;
}

static void nv_i2c_remove(struct i2c_client *client)
{
	struct nv_i2c_driver *drv = nv_i2c_drv_from_client(client);
	struct nv_i2c_dev *dev = nv_i2c_dev_from_client(client);

	if (!dev)
		return;

	if (drv && drv->ops && drv->ops->remove)
		drv->ops->remove(dev);

	i2c_set_clientdata(client, NULL);
}

static void nv_i2c_shutdown(struct i2c_client *client)
{
	struct nv_i2c_driver *drv = nv_i2c_drv_from_client(client);
	struct nv_i2c_dev *dev = nv_i2c_dev_from_client(client);

	if (drv && drv->ops && drv->ops->shutdown && dev)
		drv->ops->shutdown(dev);
}

static void nv_i2c_alert(struct i2c_client *client, enum i2c_alert_protocol protocol,
			   unsigned int data)
{
	struct nv_i2c_driver *drv = nv_i2c_drv_from_client(client);
	struct nv_i2c_dev *dev = nv_i2c_dev_from_client(client);

	if (drv && drv->ops && drv->ops->alert && dev)
		drv->ops->alert(dev, protocol, data);
}

int nv_i2c_driver_register(struct nv_i2c_driver *drv)
{
	int ret;

	if (!drv || !drv->name || !drv->owner || !drv->id_table || !drv->ops)
		return -EINVAL;

	if (drv->registered)
		return -EBUSY;

	drv->i2c_driver.driver.name = drv->name;
	drv->i2c_driver.driver.owner = drv->owner;
	drv->i2c_driver.probe = nv_i2c_probe;
	drv->i2c_driver.remove = nv_i2c_remove;
	drv->i2c_driver.shutdown = nv_i2c_shutdown;
	drv->i2c_driver.alert = nv_i2c_alert;
	drv->i2c_driver.id_table = drv->id_table;

	ret = i2c_register_driver(drv->owner, &drv->i2c_driver);
	if (ret)
		return ret;

	drv->registered = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_i2c_driver_register);

void nv_i2c_driver_unregister(struct nv_i2c_driver *drv)
{
	if (!drv || !drv->registered)
		return;

	i2c_del_driver(&drv->i2c_driver);
	drv->registered = false;
}
EXPORT_SYMBOL_GPL(nv_i2c_driver_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("I2C device framework");
MODULE_LICENSE("GPL");
