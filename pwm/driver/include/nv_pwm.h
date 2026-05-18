/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - PWM controller helper layer
 */
#ifndef _NV_PWM_H_
#define _NV_PWM_H_

#include <linux/device.h>
#include <linux/module.h>
#include <linux/pwm.h>
#include <linux/types.h>

struct nv_pwm_chip;

/**
 * struct nv_pwm_chip_ops - PWM controller callbacks
 */
struct nv_pwm_chip_ops {
	int (*apply)(struct nv_pwm_chip *chip, struct pwm_device *pwm,
		     const struct pwm_state *state);
	int (*get_state)(struct nv_pwm_chip *chip, struct pwm_device *pwm,
			 struct pwm_state *state);
	int (*request)(struct nv_pwm_chip *chip, struct pwm_device *pwm);
	void (*free)(struct nv_pwm_chip *chip, struct pwm_device *pwm);
};

/**
 * struct nv_pwm_chip - PWM chip wrapper
 */
struct nv_pwm_chip {
	struct pwm_chip *chip;
	const struct nv_pwm_chip_ops *ops;
	void *priv;
	struct device *parent;
	unsigned int npwm;
	bool added;
};

int nv_pwm_chip_register(struct device *parent, struct nv_pwm_chip *nchip,
			 unsigned int npwm);
void nv_pwm_chip_unregister(struct nv_pwm_chip *nchip);

struct nv_pwm_chip *nv_pwm_chip_from_chip(struct pwm_chip *chip);
struct pwm_device *nv_pwm_chip_get_pwm(struct nv_pwm_chip *nchip,
				       unsigned int index);

int nv_pwm_config(struct nv_pwm_chip *nchip, unsigned int index,
		  u64 period_ns, u64 duty_ns, bool enabled);
int nv_pwm_enable(struct nv_pwm_chip *nchip, unsigned int index, bool enable);
int nv_pwm_get_config(struct nv_pwm_chip *nchip, unsigned int index,
		      struct pwm_state *state);

static inline struct device *nv_pwm_chip_device(struct nv_pwm_chip *nchip)
{
	return nchip && nchip->chip ? &nchip->chip->dev : NULL;
}

#endif /* _NV_PWM_H_ */
