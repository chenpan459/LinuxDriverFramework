# nv_sdio — SDIO 功能驱动框架

封装 Linux **MMC/SDIO** 子系统的 `struct sdio_driver` 注册，以及常用 SDIO I/O 辅助函数。

## 目录结构

```
bus/sdio/
├── driver/
│   ├── include/nv_sdio.h
│   ├── core/nv_sdio.c
│   ├── modules/nv_sdio_demo.c
│   └── Makefile
├── app/
│   ├── nv_test_sdio.c
│   ├── nv_test_sdio_sysfs.sh
│   └── Makefile
└── Makefile
```

## 框架流程

```
nv_sdio_driver_register()
  └─ __sdio_register_driver()
        └─ nv_sdio_probe()
              ├─ devm_kzalloc(nv_sdio_dev)
              └─ ops->probe()
                    └─ nv_sdio_enable_func / readb / writeb ...
```

## 编译

```bash
make -C bus/sdio
```

需要内核已启用 MMC/SDIO（`CONFIG_MMC`、`CONFIG_MMC_SDIO`）。

## 测试

**无匹配 SDIO 功能时**（常见）：模块可加载，驱动出现在 sysfs，probe 不会被调用。

```bash
make -C bus/sdio
sudo insmod bus/sdio/driver/nv_sdio_demo.ko
ls /sys/bus/sdio/drivers/nv_sdio_demo
bus/sdio/app/bin/nv_test_sdio
sudo rmmod nv_sdio_demo
```

**一键脚本**：

```bash
sudo bus/sdio/app/bin/nv_test_sdio_sysfs
```

**有匹配硬件时**：demo 匹配 `vendor=0x1234` `device=0x5678`，绑定后可通过 sysfs 访问：

```bash
# 示例路径因主机而异
echo 0x5a > /sys/bus/sdio/devices/mmc1:1234:1/reg
cat /sys/bus/sdio/devices/mmc1:1234:1/reg
bus/sdio/app/bin/nv_test_sdio /sys/bus/sdio/devices/mmc1:1234:1
```

可将 `demo_ids` 中的 `SDIO_DEVICE()` 改为实际芯片的 vendor/device。

## API 摘要

| 接口 | 说明 |
|------|------|
| `nv_sdio_driver_register` | 注册 SDIO 驱动 |
| `nv_sdio_enable_func` / `nv_sdio_disable_func` | 使能/关闭功能 |
| `nv_sdio_set_block_size` | 设置块大小 |
| `nv_sdio_readb` / `nv_sdio_writeb` | 字节寄存器读写 |
| `nv_sdio_readw/writew`, `readl/writel` | 多字节访问 |
| `nv_sdio_memcpy_fromio` / `nv_sdio_memcpy_toio` | 块传输 |
| `nv_sdio_claim_host` / `nv_sdio_release_host` | 主机总线锁 |
| `nv_sdio_claim_irq` / `nv_sdio_release_irq` | SDIO 中断 |

### nv_sdio_ops

| 回调 | 说明 |
|------|------|
| `probe` | 绑定 SDIO function 时调用 |
| `remove` | 解绑时调用 |

## 编写新驱动

```c
static const struct sdio_device_id my_ids[] = {
	{ SDIO_DEVICE(0xVVVV, 0xDDDD), .driver_data = (kernel_ulong_t)&my_driver },
	{ }
};

static int my_probe(struct nv_sdio_dev *dev, const struct sdio_device_id *id)
{
	return nv_sdio_enable_func(dev);
}

static const struct nv_sdio_ops my_ops = {
	.probe = my_probe,
	.remove = my_remove,
};

nv_sdio_driver_register(&my_driver);
```
