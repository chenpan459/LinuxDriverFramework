# nv_spi — SPI 设备驱动框架

封装 `struct spi_driver` 注册、`spi_setup` 与同步传输辅助函数。

## 目录结构

```
spi/
├── driver/
│   ├── include/nv_spi.h
│   ├── core/nv_spi.c
│   ├── modules/nv_spi_demo.c
│   └── Makefile
├── app/
│   ├── nv_test_spi.c
│   ├── nv_test_spi_sysfs.sh
│   └── Makefile
└── Makefile
```

## 框架流程

```
nv_spi_driver_register()
  └─ spi_register_driver()
        └─ nv_spi_probe()
              ├─ spi_setup()
              └─ ops->probe()
```

## 编译

```bash
make -C spi
```

## 测试

### 仅加载模块

```bash
sudo insmod spi/driver/nv_spi_demo.ko
sudo spi/app/bin/nv_test_spi_sysfs
```

### 有 SPI 控制器时（设备树示例）

```dts
&spi0 {
    status = "okay";
    nv_spi_demo@0 {
        compatible = "nv,spi-demo";
        reg = <0>;
        spi-max-frequency = <1000000>;
    };
};
```

```bash
sudo insmod spi/driver/nv_spi_demo.ko
spi/app/bin/nv_test_spi
```

## API 摘要

| 接口 | 说明 |
|------|------|
| `nv_spi_driver_register` | 注册 SPI 驱动 |
| `nv_spi_setup` | `spi_setup` 封装 |
| `nv_spi_write` / `nv_spi_read` | 同步读写 |
| `nv_spi_write_then_read` | 写后读 |
| `nv_spi_xfer` | 单次全双工传输 |
| `nv_spi_set_speed` / `nv_spi_set_mode` | 运行时参数 |

### nv_spi_ops

| 回调 | 说明 |
|------|------|
| `probe` | 必需 |
| `remove` / `shutdown` | 可选 |

## 编写新驱动

```c
static const struct spi_device_id my_ids[] = {
	{ "my_spi_chip", (kernel_ulong_t)&my_driver },
	{ }
};

static int my_probe(struct nv_spi_dev *dev, const struct spi_device_id *id)
{
	return nv_spi_write(dev, buf, len);
}
```
