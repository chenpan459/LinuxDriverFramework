# nv_char — 字符设备驱动框架（cdev）

Linux 字符设备框架：cdev、安全卸载、ioctl/poll、debugfs、tracepoint、devm、platform、miscdevice。

## 目录结构

```
char/
├── include/
│   ├── nv_char.h
│   └── trace/events/nv_char.h   # tracepoint 定义
├── core/nv_char.c
├── examples/
│   ├── nv_char_echo.c
│   ├── nv_char_demo.c
│   ├── nv_char_plat.c          # platform + devm
│   ├── nv_char_misc_echo.c
│   └── nv_char_selftest.c      # 加载即自检
└── Makefile
```

## 示例模块

| 模块 | 能力 |
|------|------|
| `nv_char_echo.ko` | 标准 cdev |
| `nv_char_demo.ko` | poll + ioctl 表 |
| `nv_char_plat.ko` | platform + **devm** + OF `nv,char-plat` |
| `nv_char_misc_echo.ko` | miscdevice |
| `nv_char_selftest.ko` | 注册/设备号自检 |

## 编译与测试

```bash
cd char && make

sudo insmod nv_char_selftest.ko    # dmesg 应见 PASS
sudo rmmod nv_char_selftest

sudo insmod nv_char_plat.ko
echo hi > /dev/nv_plat && cat /dev/nv_plat
sudo rmmod nv_char_plat
```

## devm（platform 推荐）

```c
/* probe */
nv_char_driver_register_devm(&pdev->dev, &drv);
nv_char_device_register_parent(&drv, &dev, 0, "nv_plat", &ops, priv,
                               &pdev->dev);

/* remove */
nv_char_device_unregister(&dev);
nv_char_driver_unregister_devm(&drv);
/* chrdev 区与 class 在 device 解绑时由 devm 自动释放 */
```

## Tracepoint

需内核 `CONFIG_TRACING=y`：

```bash
sudo insmod nv_char_echo.ko
echo 1 | sudo tee /sys/kernel/tracing/events/nv_char/enable
echo hello > /dev/nv_echo
cat /sys/kernel/tracing/trace | grep nv_char
```

事件：`nv_char_register`、`nv_char_unregister`、`nv_char_open`、`nv_char_release`。

## debugfs

`/sys/kernel/debug/nv_char/<device>/` → `opens`、`removing`、`devt`

## API 摘要

| API | 说明 |
|-----|------|
| `nv_char_driver_register` / `_unregister` | 手动管理 |
| `nv_char_driver_register_devm` / `_unregister_devm` | 绑定 `struct device` |
| `nv_char_device_register_parent` | 指定 parent（platform） |
| `nv_char_misc_register` | miscdevice |
| `nv_char_ioctl_dispatch` | ioctl 表 |
| `nv_char_wake` | 唤醒 poll |

## 许可证

GPL-2.0
