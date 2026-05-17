# nv_block — 块设备驱动框架

封装 `gendisk`、`blk_alloc_disk`、`add_disk` 及 `block_device_operations`（基于 `submit_bio`），驱动只需实现 `nv_block_ops` 回调。

## 目录结构

```
block/
├── include/nv_block.h       # 公共 API
├── core/nv_block.c          # 框架实现
├── examples/nv_block_ram.c  # 示例：内存 RAM 盘
└── Makefile
```

## 编译

```bash
cd block
make
```

生成 `nv_block_ram.ko`（内含 `core/nv_block.o`）。

## 测试示例

```bash
sudo insmod nv_block_ram.ko              # 默认 8MB
sudo insmod nv_block_ram.ko size_mb=16   # 指定大小（1–1024 MB）

sudo dd if=/dev/zero of=/dev/nv_ram bs=4K count=1
sudo dd if=/dev/nv_ram of=/tmp/out bs=4K count=1

sudo rmmod nv_block_ram
```

### 模块参数

| 参数 | 说明 | 默认 |
|------|------|------|
| `size_mb` | 磁盘容量（MB） | 8 |

## API 概览

| 函数 | 说明 |
|------|------|
| `nv_block_driver_register()` | 初始化驱动（设备表） |
| `nv_block_driver_unregister()` | 注销驱动及其下所有磁盘 |
| `nv_block_device_register()` | 注册块设备 → `/dev/<name>` |
| `nv_block_device_unregister()` | 注销单块磁盘 |

### 数据结构

- **`nv_block_driver`**：驱动级（`name`、`owner`、`count`）
- **`nv_block_dev`**：磁盘实例（`gendisk`、`capacity`、`priv`）
- **`nv_block_ops`**：回调；**`submit_bio` 为必需**，其余可选：`open`、`release`、`ioctl`

框架在 `submit_bio` 中调用驱动并执行 `bio_endio`。

## 编写新驱动

```c
#include "nv_block.h"

static blk_status_t my_submit_bio(struct nv_block_dev *dev, struct bio *bio)
{
    /* 处理 bio */
    return BLK_STS_OK;
}

static const struct nv_block_ops my_ops = {
    .submit_bio = my_submit_bio,
};

static int __init my_init(void)
{
    my_drv.name = "my_block_drv";
    my_drv.owner = THIS_MODULE;
    my_drv.count = 1;

    nv_block_driver_register(&my_drv);
    return nv_block_device_register(&my_drv, &my_dev, 0, "my_disk",
                                    &my_ops, priv, sectors, 512);
}
```

`Makefile`：

```makefile
obj-m := my_driver.o
my_driver-y := my_driver.o core/nv_block.o
ccflags-y += -I$(src)/include
```

## 清理

```bash
make clean
```
