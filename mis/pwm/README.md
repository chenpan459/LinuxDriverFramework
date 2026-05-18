# nv_pwm — PWM 控制器驱动框架

封装 `pwmchip_alloc` / `pwmchip_add` 与 `pwm_ops`（`apply` / `get_state`）分发。

## 目录结构

```
pwm/
├── driver/
│   ├── include/nv_pwm.h
│   ├── core/nv_pwm.c
│   ├── modules/nv_pwm_demo.c
│   └── Makefile
├── app/
│   ├── nv_test_pwm.c
│   ├── nv_test_pwm_sysfs.sh
│   └── Makefile
└── Makefile
```

## 框架流程

```
nv_pwm_chip_register()
  └─ pwmchip_alloc() + pwmchip_add()
        └─ nv_pwm_ops.apply / get_state
              └─ nv_pwm_chip_ops (驱动实现)
```

## 编译

```bash
make -C pwm
```

## 测试（软件 PWM demo）

```bash
sudo insmod pwm/driver/nv_pwm_demo.ko
# /sys/class/pwm/pwmchipN/
pwm/app/bin/nv_test_pwm

# 或
sudo pwm/app/bin/nv_test_pwm_sysfs
```

## API 摘要

| 接口 | 说明 |
|------|------|
| `nv_pwm_chip_register` | 注册 PWM 控制器 |
| `nv_pwm_chip_unregister` | 注销 |
| `nv_pwm_config` | 配置 period/duty/enable |
| `nv_pwm_enable` | 开关 PWM |
| `nv_pwm_get_config` | 读取当前状态 |

### nv_pwm_chip_ops

| 回调 | 说明 |
|------|------|
| `apply` | 必需，应用 `pwm_state` |
| `get_state` | 必需，读取硬件/逻辑状态 |
| `request` / `free` | 可选 |

## 编写新驱动

```c
static int my_apply(struct nv_pwm_chip *chip, struct pwm_device *pwm,
		    const struct pwm_state *state)
{
	/* 配置硬件 */
	return 0;
}

static const struct nv_pwm_chip_ops my_ops = {
	.apply = my_apply,
	.get_state = my_get_state,
};

nv_pwm_chip_register(&pdev->dev, &my_chip, npwm);
```

设备树：`compatible = "nv,pwm-demo"`（与 demo 相同模式可扩展）。
