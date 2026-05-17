// SPDX-License-Identifier: GPL-2.0
/*
 * Example: software PWM chip using nv_pwm framework
 *
 * Exposes /sys/class/pwm/pwmchipN/ with 4 channels (in-memory state).
 *
 * Load:  sudo insmod nv_pwm_demo.ko
 * Test:  pwm/app/bin/nv_test_pwm
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>

#include "nv_pwm.h"

#define DRV_NAME	"nv_pwm_demo"
#define NV_PWM_NPWM	4

struct demo_chan {
	struct pwm_state state;
};

struct demo_priv {
	struct demo_chan ch[NV_PWM_NPWM];
};

static struct nv_pwm_chip demo_chip;
static struct demo_priv demo_data;
static struct platform_device *demo_pdev;

static int demo_apply(struct nv_pwm_chip *nchip, struct pwm_device *pwm,
		      const struct pwm_state *state)
{
	struct demo_priv *priv = nchip->priv;
	unsigned int idx = pwm->hwpwm;

	if (idx >= NV_PWM_NPWM)
		return -EINVAL;

	if (state->duty_cycle > state->period && state->period)
		return -EINVAL;

	priv->ch[idx].state = *state;
	return 0;
}

static int demo_get_state(struct nv_pwm_chip *nchip, struct pwm_device *pwm,
			  struct pwm_state *state)
{
	struct demo_priv *priv = nchip->priv;
	unsigned int idx = pwm->hwpwm;

	if (idx >= NV_PWM_NPWM || !state)
		return -EINVAL;

	*state = priv->ch[idx].state;
	return 0;
}

static const struct nv_pwm_chip_ops demo_ops = {
	.apply		= demo_apply,
	.get_state	= demo_get_state,
};

static int demo_register_chip(struct device *parent)
{
	int ret;

	demo_chip.ops = &demo_ops;
	demo_chip.priv = &demo_data;

	ret = nv_pwm_chip_register(parent, &demo_chip, NV_PWM_NPWM);
	if (ret)
		return ret;

	dev_info(parent, "nv_pwm_demo: pwmchip ready (%u PWMs)\n", NV_PWM_NPWM);
	return 0;
}

static void demo_unregister_chip(void)
{
	nv_pwm_chip_unregister(&demo_chip);
}

static int demo_plat_probe(struct platform_device *pdev)
{
	return demo_register_chip(&pdev->dev);
}

static void demo_plat_remove(struct platform_device *pdev)
{
	demo_unregister_chip();
}

static const struct of_device_id demo_of_match[] = {
	{ .compatible = "nv,pwm-demo", },
	{ }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct platform_driver demo_plat_driver = {
	.probe	= demo_plat_probe,
	.remove	= demo_plat_remove,
	.driver	= {
		.name	= DRV_NAME,
		.of_match_table = demo_of_match,
	},
};

static int __init demo_init(void)
{
	int ret;

	ret = platform_driver_register(&demo_plat_driver);
	if (ret)
		return ret;

	demo_pdev = platform_device_register_simple(DRV_NAME, -1, NULL, 0);
	if (IS_ERR(demo_pdev)) {
		platform_driver_unregister(&demo_plat_driver);
		return PTR_ERR(demo_pdev);
	}

	return 0;
}

static void __exit demo_exit(void)
{
	platform_device_unregister(demo_pdev);
	platform_driver_unregister(&demo_plat_driver);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_IMPORT_NS("PWM");
MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("PWM demo chip (software)");
MODULE_LICENSE("GPL");
