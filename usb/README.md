# nv_usb — USB 设备驱动框架

封装 `struct usb_driver` 注册、altsetting 选择、端点发现与同步 bulk/control 传输，驱动只需实现 `nv_usb_ops`。

## 目录结构

```
usb/
├── include/nv_usb.h
├── core/nv_usb.c
├── examples/nv_usb_demo.c   # 匹配 usb-skeleton 测试 ID (0525:a4a7)
└── Makefile
```

## 框架流程

```
nv_usb_driver_register()
  └─ usb_register_driver()
        └─ nv_usb_probe()
              ├─ devm_kzalloc(nv_usb_dev)
              ├─ nv_usb_select_altsetting(0)   # 可选跳过
              └─ ops->probe()

nv_usb_disconnect()
  ├─ ops->disconnect()
  └─ usb_set_intfdata(NULL)
```

## 编译

```bash
cd usb && make
```

## 测试

无匹配 USB 设备时模块可正常加载，但不会触发 `probe`。

使用 **dummy_hcd** + 具备 bulk 端点的 gadget，或插入 VID/PID 为 `0525:a4a7` 的测试设备（与内核 usb-skeleton 示例相同）：

```bash
sudo modprobe dummy_hcd
# 配置 gadget 或使用 usb-skeleton 硬件
sudo insmod nv_usb_demo.ko
dmesg | grep nv_usb_demo
sudo rmmod nv_usb_demo
```

## API 摘要

| 接口 | 说明 |
|------|------|
| `nv_usb_driver_register` | 注册 USB 接口驱动 |
| `nv_usb_driver_unregister` | 注销 |
| `nv_usb_find_endpoints` | 解析 bulk in/out、interrupt in |
| `nv_usb_bulk_in` / `nv_usb_bulk_out` | 同步 bulk 传输 |
| `nv_usb_control` | 控制传输 |
| `nv_usb_select_altsetting` | `usb_set_interface` 封装 |

### nv_usb_ops

| 回调 | 说明 |
|------|------|
| `probe` | 必需 |
| `disconnect` | 可选 |
| `suspend` / `resume` / `reset_resume` / `shutdown` | 电源管理 |

### 标志位

```c
drv->flags = NV_USB_FLAG_ENABLE_AUTOSUSPEND;
drv->flags = NV_USB_FLAG_SKIP_ALTSETTING;  /* 不强制 alt 0 */
```

## 编写新驱动

```c
static int my_probe(struct nv_usb_dev *dev, const struct usb_device_id *id)
{
    nv_usb_find_endpoints(dev);
    /* 使用 dev->eps.bulk_in_addr 等 */
    return 0;
}

static const struct nv_usb_ops my_ops = {
    .probe = my_probe,
    .disconnect = my_remove,
};

static struct nv_usb_driver my_driver;

static const struct usb_device_id my_ids[] = {
    { USB_DEVICE(VENDOR, PRODUCT), .driver_info = (kernel_ulong_t)&my_driver },
    { }
};

static int __init my_init(void)
{
    my_driver.name = "my_usb";
    my_driver.owner = THIS_MODULE;
    my_driver.id_table = my_ids;
    my_driver.ops = &my_ops;
    return nv_usb_driver_register(&my_driver);
}
```
