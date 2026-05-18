# nv_usb — USB 设备驱动框架

封装 `struct usb_driver` 注册、altsetting 选择、端点发现与同步 bulk/control 传输，驱动只需实现 `nv_usb_ops`。

## 目录结构

```
drivers/usb/
├── include/nv_usb.h
├── core/nv_usb.c
├── modules/nv_usb_demo.c
└── Makefile
```

用户态测试见 [apps/usb/](../../apps/usb/)。

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

```bash
make -C drivers/usb && make -C apps/usb
sudo apps/usb/bin/nv_test_usb
```

无匹配 USB 设备时模块可加载但 `probe` 不执行；有 `0525:a4a7` 设备或 dummy_hcd gadget 时 dmesg 可见 probe 信息。

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
