// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - PWM helper implementation
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pwm.h>

#include "nv_pwm.h"

MODULE_IMPORT_NS("PWM");

struct nv_pwm_chip *nv_pwm_chip_from_chip(struct pwm_chip *chip)
{
	return chip ? pwmchip_get_drvdata(chip) : NULL;
}
EXPORT_SYMBOL_GPL(nv_pwm_chip_from_chip);

struct pwm_device *nv_pwm_chip_get_pwm(struct nv_pwm_chip *nchip,
				       unsigned int index)
{
	if (!nchip || !nchip->chip || index >= nchip->npwm)
		return NULL;

	return &nchip->chip->pwms[index];
}
EXPORT_SYMBOL_GPL(nv_pwm_chip_get_pwm);

int nv_pwm_config(struct nv_pwm_chip *nchip, unsigned int index,
		  u64 period_ns, u64 duty_ns, bool enabled)
{
	struct pwm_device *pwm;
	struct pwm_state state;

	if (!nchip || !nchip->chip)
		return -EINVAL;

	pwm = nv_pwm_chip_get_pwm(nchip, index);
	if (!pwm)
		return -EINVAL;

	if (duty_ns > period_ns && period_ns)
		return -EINVAL;

	pwm_init_state(pwm, &state);
	state.period = period_ns;
	state.duty_cycle = duty_ns;
	state.enabled = enabled;
	state.polarity = PWM_POLARITY_NORMAL;

	return pwm_apply_might_sleep(pwm, &state);
}
EXPORT_SYMBOL_GPL(nv_pwm_config);

int nv_pwm_enable(struct nv_pwm_chip *nchip, unsigned int index, bool enable)
{
	struct pwm_device *pwm;
	struct pwm_state state;

	if (!nchip || !nchip->chip)
		return -EINVAL;

	pwm = nv_pwm_chip_get_pwm(nchip, index);
	if (!pwm)
		return -EINVAL;

	if (nchip->ops && nchip->ops->get_state) {
		int ret = nchip->ops->get_state(nchip, pwm, &state);

		if (ret)
			return ret;
	} else {
		pwm_get_state(pwm, &state);
	}

	state.enabled = enable;
	return pwm_apply_might_sleep(pwm, &state);
}
EXPORT_SYMBOL_GPL(nv_pwm_enable);

int nv_pwm_get_config(struct nv_pwm_chip *nchip, unsigned int index,
		      struct pwm_state *state)
{
	struct pwm_device *pwm;

	if (!nchip || !nchip->chip || !state)
		return -EINVAL;

	pwm = nv_pwm_chip_get_pwm(nchip, index);
	if (!pwm)
		return -EINVAL;

	if (nchip->ops && nchip->ops->get_state)
		return nchip->ops->get_state(nchip, pwm, state);

	pwm_get_state(pwm, state);
	return 0;
}
EXPORT_SYMBOL_GPL(nv_pwm_get_config);

static int nv_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			const struct pwm_state *state)
{
	struct nv_pwm_chip *nchip = nv_pwm_chip_from_chip(chip);

	if (!nchip || !nchip->ops || !nchip->ops->apply)
		return -ENODEV;

	return nchip->ops->apply(nchip, pwm, state);
}

static int nv_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
			    struct pwm_state *state)
{
	struct nv_pwm_chip *nchip = nv_pwm_chip_from_chip(chip);

	if (!nchip || !nchip->ops || !nchip->ops->get_state)
		return -ENODEV;

	return nchip->ops->get_state(nchip, pwm, state);
}

static int nv_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct nv_pwm_chip *nchip = nv_pwm_chip_from_chip(chip);

	if (nchip && nchip->ops && nchip->ops->request)
		return nchip->ops->request(nchip, pwm);

	return 0;
}

static void nv_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct nv_pwm_chip *nchip = nv_pwm_chip_from_chip(chip);

	if (nchip && nchip->ops && nchip->ops->free)
		nchip->ops->free(nchip, pwm);
}

static const struct pwm_ops nv_pwm_core_ops = {
	.request	= nv_pwm_request,
	.free		= nv_pwm_free,
	.apply		= nv_pwm_apply,
	.get_state	= nv_pwm_get_state,
};

int nv_pwm_chip_register(struct device *parent, struct nv_pwm_chip *nchip,
			 unsigned int npwm)
{
	struct pwm_chip *chip;
	int ret;

	if (!parent || !nchip || !nchip->ops || !nchip->ops->apply ||
	    !nchip->ops->get_state || !npwm)
		return -EINVAL;

	if (nchip->added)
		return -EBUSY;

	chip = pwmchip_alloc(parent, npwm, 0);
	if (!chip)
		return -ENOMEM;

	chip->ops = &nv_pwm_core_ops;
	pwmchip_set_drvdata(chip, nchip);

	nchip->chip = chip;
	nchip->parent = parent;
	nchip->npwm = npwm;

	ret = pwmchip_add(chip);
	if (ret) {
		pwmchip_put(chip);
		nchip->chip = NULL;
		return ret;
	}

	nchip->added = true;
	dev_info(parent, "nv_pwm: chip registered (%u channels)\n", npwm);
	return 0;
}
EXPORT_SYMBOL_GPL(nv_pwm_chip_register);

void nv_pwm_chip_unregister(struct nv_pwm_chip *nchip)
{
	if (!nchip || !nchip->added || !nchip->chip)
		return;

	pwmchip_remove(nchip->chip);
	pwmchip_put(nchip->chip);
	nchip->chip = NULL;
	nchip->added = false;
}
EXPORT_SYMBOL_GPL(nv_pwm_chip_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("PWM controller framework");
MODULE_LICENSE("GPL");
