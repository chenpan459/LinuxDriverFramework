# nv_usb — USB 驱动框架

## 目录结构

```
usb/
├── driver/
├── app/
└── Makefile
```

## 编译与测试

```bash
make -C usb
sudo app/bin/nv_test_usb
```

匹配测试 ID：`0525:a4a7`（usb-skeleton）。
