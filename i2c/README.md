# nv_i2c — I2C 设备驱动框架

封装 `struct i2c_driver` 注册与 SMBus/主模式读写辅助函数。

## 目录结构

```
i2c/
├── driver/
│   ├── include/nv_i2c.h
│   ├── core/nv_i2c.c
│   ├── modules/nv_i2c_demo.c
│   └── Makefile
├── app/
│   ├── nv_test_i2c.c          # sysfs reg 测试
│   ├── nv_test_i2c_sysfs.sh   # 完整流程（i2c-stub）
│   └── Makefile
└── Makefile
```

## 框架流程

```
nv_i2c_driver_register()
  └─ i2c_register_driver()
        └─ nv_i2c_probe()
              ├─ devm_kzalloc(nv_i2c_dev)
              └─ ops->probe()
```

## 编译

```bash
make -C i2c
```

## 测试（无硬件，使用 i2c-stub）

```bash
sudo modprobe i2c_stub chip_addr=0x50
make -C i2c
sudo insmod i2c/driver/nv_i2c_demo.ko
echo nv_i2c_demo 0x50 > /sys/bus/i2c/devices/i2c-0/new_device
i2c/app/bin/nv_test_i2c
# 或一键：
sudo i2c/app/bin/nv_test_i2c_sysfs
```

## API 摘要

| 接口 | 说明 |
|------|------|
| `nv_i2c_driver_register` | 注册 I2C 驱动 |
| `nv_i2c_read` / `nv_i2c_write` | 主模式收发 |
| `nv_i2c_smbus_*` | SMBus byte/word 读写 |

### nv_i2c_ops

| 回调 | 说明 |
|------|------|
| `probe` | 必需 |
| `remove` | 可选 |
| `shutdown` / `alert` | 可选 |

## 编写新驱动

```c
static const struct i2c_device_id my_ids[] = {
	{ "my_chip", (kernel_ulong_t)&my_driver },
	{ }
};

static int my_probe(struct nv_i2c_dev *dev, const struct i2c_device_id *id)
{
	return nv_i2c_smbus_read_byte_data(dev, 0x00);
}
```

设备树：`compatible = "vendor,my-chip"` 需同时实现 `of_match` 与 `i2c_device_id`（与 demo 相同模式）。
