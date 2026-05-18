// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - ADC (IIO) helper implementation
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

#include "nv_adc.h"

struct nv_adc_dev *nv_adc_dev_from_indio(struct iio_dev *indio_dev)
{
	struct nv_adc_iio_priv *priv;

	if (!indio_dev)
		return NULL;

	priv = iio_priv(indio_dev);
	return priv ? priv->adev : NULL;
}
EXPORT_SYMBOL_GPL(nv_adc_dev_from_indio);

static int nv_adc_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan,
			   int *val, int *val2, long mask)
{
	struct nv_adc_dev *adev = nv_adc_dev_from_indio(indio_dev);

	if (!adev || !adev->ops || !adev->ops->read_raw)
		return -ENODEV;

	return adev->ops->read_raw(adev, chan, val, val2, mask);
}

static int nv_adc_write_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int val, int val2, long mask)
{
	struct nv_adc_dev *adev = nv_adc_dev_from_indio(indio_dev);

	if (!adev || !adev->ops || !adev->ops->write_raw)
		return -ENODEV;

	return adev->ops->write_raw(adev, chan, val, val2, mask);
}

static const struct iio_info nv_adc_iio_info = {
	.read_raw	= nv_adc_read_raw,
	.write_raw	= nv_adc_write_raw,
};

int nv_adc_register(struct device *parent, struct nv_adc_dev *adev,
		    const char *name, const struct iio_chan_spec *channels,
		    unsigned int num_channels)
{
	struct iio_dev *indio;
	struct nv_adc_iio_priv *priv;
	int ret;

	if (!parent || !adev || !name || !channels || !num_channels || !adev->ops)
		return -EINVAL;

	if (!adev->ops->read_raw)
		return -EINVAL;

	if (adev->registered)
		return -EBUSY;

	indio = iio_device_alloc(parent, sizeof(*priv));
	if (!indio)
		return -ENOMEM;

	priv = iio_priv(indio);
	priv->adev = adev;
	adev->indio_dev = indio;

	indio->dev.parent = parent;
	indio->name = name;
	indio->info = &nv_adc_iio_info;
	indio->modes = INDIO_DIRECT_MODE;
	indio->channels = channels;
	indio->num_channels = num_channels;

	ret = iio_device_register(indio);
	if (ret) {
		iio_device_free(indio);
		adev->indio_dev = NULL;
		return ret;
	}

	adev->registered = true;
	dev_info(parent, "nv_adc: %s registered (%u channels)\n",
		 name, num_channels);
	return 0;
}
EXPORT_SYMBOL_GPL(nv_adc_register);

void nv_adc_unregister(struct nv_adc_dev *dev)
{
	if (!dev || !dev->registered || !dev->indio_dev)
		return;

	iio_device_unregister(dev->indio_dev);
	iio_device_free(dev->indio_dev);
	dev->indio_dev = NULL;
	dev->registered = false;
}
EXPORT_SYMBOL_GPL(nv_adc_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("ADC (IIO) framework");
MODULE_LICENSE("GPL");
