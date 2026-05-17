# nv_char — 字符设备驱动框架

封装 `cdev`、`alloc_chrdev_region`、`class_create`、`device_create` 及 `file_operations` 分发，驱动只需实现 `nv_char_ops` 回调。

## 目录结构

```
char/
├── include/nv_char.h      # 公共 API
├── core/nv_char.c         # 框架实现
├── examples/nv_char_echo.c # 示例：内存 echo 设备
└── Makefile
```

## 编译

```bash
cd char
make
```

生成 `nv_char_echo.ko`（内含 `core/nv_char.o`）。

## 测试示例

```bash
sudo insmod nv_char_echo.ko
echo hello > /dev/nv_echo
cat /dev/nv_echo
sudo rmmod nv_char_echo
```

## API 概览

| 函数 | 说明 |
|------|------|
| `nv_char_driver_register()` | 分配主设备号、创建 class |
| `nv_char_driver_unregister()` | 注销驱动，并卸载其下所有设备 |
| `nv_char_device_register()` | 注册单个字符设备 → `/dev/<name>` |
| `nv_char_device_unregister()` | 注销单个设备 |

### 数据结构

- **`nv_char_driver`**：驱动级（`name`、`owner`、`count`、主设备号、class）
- **`nv_char_dev`**：设备实例（`cdev`、`device`、`priv`）
- **`nv_char_ops`**：可选回调：`open`、`release`、`read`、`write`、`unlocked_ioctl`、`compat_ioctl`、`poll`、`llseek`

未实现的回调有默认行为（例如未提供 `read`/`write` 返回 `-ENODEV`）。

## 编写新驱动

```c
#include "nv_char.h"

static struct nv_char_driver my_drv;
static struct nv_char_dev my_dev;

static const struct nv_char_ops my_ops = {
    .read  = my_read,
    .write = my_write,
};

static int __init my_init(void)
{
    my_drv.name = "my_char_drv";
    my_drv.owner = THIS_MODULE;
    my_drv.count = 1;

    nv_char_driver_register(&my_drv);
    return nv_char_device_register(&my_drv, &my_dev, 0,
                                   "my_dev", &my_ops, priv);
}
```

`Makefile` 中将 `core/nv_char.o` 链入你的模块：

```makefile
obj-m := my_driver.o
my_driver-y := my_driver.o core/nv_char.o
ccflags-y += -I$(src)/include
```

## 清理

```bash
make clean
```
