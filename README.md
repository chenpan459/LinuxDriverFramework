# LinuxDriverFramework

Linux 内核驱动框架集合，按设备类型分子目录，统一使用 `nv_` 前缀命名。

| 目录 | 说明 | 示例模块 |
|------|------|----------|
| [char/](char/) | 字符设备（cdev/devm/trace/平台） | echo、demo、plat、misc_echo、selftest |
| [block/](block/) | 块设备框架 | `nv_block_ram.ko` → `/dev/nv_ram` |
| [net/](net/) | 网络设备框架 | `nv_net_dummy.ko` → `nv0` |
| [pci/](pci/) | PCI/PCIe 驱动框架 | `nv_pci_demo.ko`（QEMU edu） |
| [usb/](usb/) | USB 接口驱动框架 | `nv_usb_demo.ko`（0525:a4a7） |

## 环境要求

- Linux 内核头文件（与当前运行内核版本一致）
- `gcc`、`make`

```bash
# Ubuntu / Debian
sudo apt install build-essential linux-headers-$(uname -r)
```

## 快速开始

各子目录独立编译，详见各自 `README.md`：

```bash
make -C char && sudo insmod char/nv_char_echo.ko
make -C block && sudo insmod block/nv_block_ram.ko
make -C net && sudo insmod net/nv_net_dummy.ko
make -C pci && sudo insmod pci/nv_pci_demo.ko
make -C usb && sudo insmod usb/nv_usb_demo.ko
```

## 目录结构

```
LinuxDriverFramework/
├── char/          # 字符设备：cdev + sysfs
├── block/         # 块设备：gendisk + bio
├── net/           # 网络设备：net_device
├── pci/           # PCI/PCIe：pci_driver + BAR/IRQ
├── usb/           # USB：usb_driver + 端点/bulk
├── .gitignore
└── README.md
```

## Git 仓库管理

本项目使用 Git 进行版本管理。克隆、分支与提交请仅在仓库根目录操作。

### 克隆与首次构建

```bash
git clone <仓库 URL> LinuxDriverFramework
cd LinuxDriverFramework

# 各子目录分别编译（产物已被 .gitignore 忽略，勿提交）
make -C char
make -C block
make -C net
```

