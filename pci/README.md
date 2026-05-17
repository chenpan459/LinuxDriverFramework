# nv_pci — PCI/PCIe 驱动框架

封装 `struct pci_driver` 注册、`pcim_enable_device`、`BAR` 映射与 MSI/legacy 中断申请，驱动只需实现 `nv_pci_ops`。

## 目录结构

```
pci/
├── include/nv_pci.h
├── core/nv_pci.c
├── examples/nv_pci_demo.c   # 匹配 QEMU edu (1234:11e9)
└── Makefile
```

## 框架流程

```
nv_pci_driver_register()
  └─ pci_register_driver()
        └─ nv_pci_probe()
              ├─ devm_kzalloc(nv_pci_dev)
              ├─ nv_pci_enable()     # pcim_enable + set_master
              └─ ops->probe()

nv_pci_remove()
  ├─ nv_pci_free_irq()
  ├─ ops->remove()
  └─ nv_pci_disable()
```

## 编译

```bash
cd pci && make
```

## 测试（QEMU edu 设备）

```bash
# 启动 QEMU 时添加: -device edu
sudo insmod nv_pci_demo.ko
dmesg | grep nv_pci_demo
sudo rmmod nv_pci_demo
```

无匹配 PCI 设备时模块可正常加载，但不会触发 `probe`。

## API 摘要

| 接口 | 说明 |
|------|------|
| `nv_pci_driver_register` | 注册 PCI 驱动 |
| `nv_pci_driver_unregister` | 注销 |
| `nv_pci_enable` / `nv_pci_disable` | 使能设备、Bus Master |
| `nv_pci_iomap_bar` | `pcim_iomap` 映射 BAR |
| `nv_pci_request_irq` | MSI 或 legacy 单向量 |
| `nv_pci_bar_start` / `nv_pci_bar_len` | BAR 资源信息 |

### nv_pci_ops

| 回调 | 说明 |
|------|------|
| `probe` | 必需；框架已完成 `nv_pci_enable` |
| `remove` | 可选 |
| `suspend` / `resume` / `shutdown` | 电源管理 |

### 标志位

```c
demo_driver.flags = NV_PCI_FLAG_SKIP_MWI;  /* 不调用 pcim_set_mwi */
```

## 编写新驱动

```c
static int my_probe(struct nv_pci_dev *dev, const struct pci_device_id *id)
{
    nv_pci_iomap_bar(dev, 0, 0);
    nv_pci_request_irq(dev, my_isr, "my_pci", 0, dev);
    return 0;
}

static const struct nv_pci_ops my_ops = { .probe = my_probe, .remove = my_remove };

static const struct pci_device_id my_ids[] = {
    { PCI_DEVICE(0xVVVV, 0xDDDD) },
    { }
};
MODULE_DEVICE_TABLE(pci, my_ids);

static struct nv_pci_driver my_driver = {
    .name = "my_pci",
    .owner = THIS_MODULE,
    .id_table = my_ids,
    .ops = &my_ops,
};
```

`Makefile`：

```makefile
obj-m := my_pci.o
my_pci-y := my_pci.o core/nv_pci.o
ccflags-y += -I$(src)/include
```

## 许可证

GPL-2.0
