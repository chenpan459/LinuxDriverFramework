# nv_pci — PCI/PCIe 驱动框架

## 目录结构

```
pci/
├── driver/
├── app/
└── Makefile
```

## 编译与测试

```bash
make -C pci
sudo app/bin/nv_test_pci
```

QEMU 测试设备：`-device edu`（1234:11e9）。
