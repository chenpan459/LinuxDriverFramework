# LinuxDriverFramework

Linux 内核驱动框架集合，按设备类型分子目录，每个子目录内再分为 **driver**（内核）与 **app**（用户态）。

## 目录结构（两级）

```
LinuxDriverFramework/
├── char/
│   ├── driver/      # 内核框架 + 示例 .ko
│   ├── app/         # 用户态测试程序
│   └── Makefile
├── block/
├── net/
├── pci/
├── usb/
├── i2c/
├── spi/
├── uart/
├── pwm/
├── can/
├── adc/
├── bus/
│   ├── pci/ usb/ i2c/ spi/ uart/ can/
│   └── sdio/          # SDIO 功能驱动框架
└── Makefile
```

## 编译

```bash
make all       # 全部子系统的 driver + app
make driver    # 仅内核模块
make app       # 仅用户态程序

make -C block          # 单个子系统
make -C block driver
make -C block/app
```

## 测试示例（block）

```bash
make -C block
sudo insmod block/driver/nv_block_ram.ko
block/app/bin/nv_test_ram
sudo rmmod nv_block_ram
```

## 子系统

| 目录 | 驱动模块 | 应用测试 |
|------|----------|----------|
| char | `driver/nv_char_*.ko` | `app/bin/nv_test_*` |
| block | `driver/nv_block_ram.ko` | `app/bin/nv_test_ram` |
| net | `driver/nv_net_dummy.ko` | `app/bin/nv_test_net` |
| pci | `driver/nv_pci_demo.ko` | `app/bin/nv_test_pci` |
| usb | `driver/nv_usb_demo.ko` | `app/bin/nv_test_usb` |
| i2c | `driver/nv_i2c_demo.ko` | `app/bin/nv_test_i2c` |
| spi | `driver/nv_spi_demo.ko` | `app/bin/nv_test_spi` |
| uart | `driver/nv_uart_demo.ko` | `app/bin/nv_test_uart` → `/dev/ttyNV0` |
| pwm | `driver/nv_pwm_demo.ko` | `app/bin/nv_test_pwm` → sysfs |
| can | `driver/nv_can_demo.ko` | `app/bin/nv_test_can` → `nvcan0` |
| adc | `driver/nv_adc_demo.ko` | `app/bin/nv_test_adc` → IIO sysfs |
| bus/sdio | `driver/nv_sdio_demo.ko` | `app/bin/nv_test_sdio` → SDIO sysfs |

## 环境

```bash
sudo apt install build-essential linux-headers-$(uname -r)
```

`*.ko` 与 `*/app/bin/` 已加入 `.gitignore`。
