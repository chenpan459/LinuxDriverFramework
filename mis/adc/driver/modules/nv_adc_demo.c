// SPDX-License-Identifier: GPL-2.0
/*
 * Example: software ADC using nv_adc framework (IIO)
 *
 * Exposes /sys/bus/iio/devices/iio:deviceN/in_voltage*_raw
 *
 * Load:  sudo insmod nv_adc_demo.ko
 * Test:  adc/app/bin/nv_test_adc
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/iio/iio.h>

#include "nv_adc.h"

#define DRV_NAME	"nv_adc_demo"
#define NV_ADC_CHANNELS	4

struct demo_priv {
	int raw[NV_ADC_CHANNELS];
};

static struct nv_adc_dev demo_adc;
static struct demo_priv demo_data;
static struct platform_device *demo_pdev;

static const struct iio_chan_spec demo_channels[] = {
	NV_ADC_CHAN_VOLTAGE(0),
	NV_ADC_CHAN_VOLTAGE(1),
	NV_ADC_CHAN_VOLTAGE(2),
	NV_ADC_CHAN_VOLTAGE(3),
};

static int demo_read_raw(struct nv_adc_dev *dev, struct iio_chan_spec const *chan,
			 int *val, int *val2, long mask)
{
	struct demo_priv *priv = dev->priv;

	if (!priv || chan->channel >= NV_ADC_CHANNELS)
		return -EINVAL;

	if (mask != IIO_CHAN_INFO_RAW)
		return -EINVAL;

	*val = priv->raw[chan->channel];
	return IIO_VAL_INT;
}

static int demo_write_raw(struct nv_adc_dev *dev, struct iio_chan_spec const *chan,
			  int val, int val2, long mask)
{
	struct demo_priv *priv = dev->priv;

	if (!priv || chan->channel >= NV_ADC_CHANNELS)
		return -EINVAL;

	if (mask != IIO_CHAN_INFO_RAW)
		return -EINVAL;

	priv->raw[chan->channel] = val;
	return 0;
}

static const struct nv_adc_ops demo_ops = {
	.read_raw	= demo_read_raw,
	.write_raw	= demo_write_raw,
};

static int __init demo_init(void)
{
	int i, ret;

	for (i = 0; i < NV_ADC_CHANNELS; i++)
		demo_data.raw[i] = (i + 1) * 1000;

	demo_adc.ops = &demo_ops;
	demo_adc.priv = &demo_data;

	demo_pdev = platform_device_register_simple(DRV_NAME, -1, NULL, 0);
	if (IS_ERR(demo_pdev))
		return PTR_ERR(demo_pdev);

	ret = nv_adc_register(&demo_pdev->dev, &demo_adc, DRV_NAME,
			      demo_channels, ARRAY_SIZE(demo_channels));
	if (ret) {
		platform_device_unregister(demo_pdev);
		return ret;
	}

	return 0;
}

static void __exit demo_exit(void)
{
	nv_adc_unregister(&demo_adc);
	platform_device_unregister(demo_pdev);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("ADC demo driver (IIO voltage channels)");
MODULE_LICENSE("GPL");
