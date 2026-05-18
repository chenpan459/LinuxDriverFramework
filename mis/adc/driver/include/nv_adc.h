/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - ADC (IIO) helper layer
 */
#ifndef _NV_ADC_H_
#define _NV_ADC_H_

#include <linux/device.h>
#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/types.h>

struct nv_adc_dev;

/**
 * struct nv_adc_ops - ADC driver callbacks
 */
struct nv_adc_ops {
	int (*read_raw)(struct nv_adc_dev *dev, struct iio_chan_spec const *chan,
			int *val, int *val2, long mask);
	int (*write_raw)(struct nv_adc_dev *dev, struct iio_chan_spec const *chan,
			 int val, int val2, long mask);
};

/**
 * struct nv_adc_iio_priv - stored in iio_priv()
 */
struct nv_adc_iio_priv {
	struct nv_adc_dev *adev;
};

/**
 * struct nv_adc_dev - one IIO ADC device
 */
struct nv_adc_dev {
	struct iio_dev *indio_dev;
	const struct nv_adc_ops *ops;
	void *priv;
	bool registered;
};

int nv_adc_register(struct device *parent, struct nv_adc_dev *dev,
		    const char *name, const struct iio_chan_spec *channels,
		    unsigned int num_channels);
void nv_adc_unregister(struct nv_adc_dev *dev);

struct nv_adc_dev *nv_adc_dev_from_indio(struct iio_dev *indio_dev);

#define NV_ADC_CHAN_VOLTAGE(_ch) {					\
	.type = IIO_VOLTAGE,						\
	.indexed = 1,							\
	.channel = (_ch),						\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),			\
}

#endif /* _NV_ADC_H_ */
