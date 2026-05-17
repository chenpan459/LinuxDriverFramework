# nv_net — 网络设备驱动框架

封装 `alloc_etherdev`、`register_netdev` 及 `net_device_ops` 分发，驱动只需实现 `nv_net_ops` 回调。

## 目录结构

```
drivers/net/
├── include/nv_net.h
├── core/nv_net.c
├── modules/nv_net_dummy.c
└── Makefile
```

用户态测试见 [apps/net/](../../apps/net/)。

## 编译

```bash
cd net
make
```

生成 `nv_net_dummy.ko`（内含 `core/nv_net.o`）。

## 测试

```bash
make -C drivers/net && make -C apps/net
sudo insmod drivers/net/nv_net_dummy.ko
apps/net/bin/nv_test_net
sudo rmmod nv_net_dummy
```

接口名由 `dev_alloc_name` 分配，默认模板为 `nv%d`（首个为 `nv0`）。

## API 概览

| 函数 | 说明 |
|------|------|
| `nv_net_driver_register()` | 初始化驱动（设备表） |
| `nv_net_driver_unregister()` | 注销驱动及其下所有网卡 |
| `nv_net_device_register()` | 注册网卡 |
| `nv_net_device_unregister()` | 注销单张网卡 |

### 数据结构

- **`nv_net_driver`**：驱动级（`name`、`owner`、`count`）
- **`nv_net_dev`**：网卡实例（`netdev`、`priv`）
- **`nv_net_ops`**：回调；发送路径需实现 **`start_xmit`**，其余可选：`open`、`stop`、`set_mac_address`、`change_mtu`、`set_rx_mode`、`ioctl`、`get_stats64`

未提供 `open`/`stop` 时，框架默认启停 TX 队列；未提供 `start_xmit` 时丢弃报文。

### `nv_net_device_register` 参数

| 参数 | 说明 |
|------|------|
| `mac_addr` | MAC 地址，传 `NULL` 则随机分配 |
| `mtu` | MTU，传 `0` 或负数使用 `ETH_DATA_LEN` |
| `name` | 接口名模板，如 `"nv%d"`、`"myeth%d"` |

## 编写新驱动

```c
#include "nv_net.h"

static netdev_tx_t my_xmit(struct nv_net_dev *dev, struct sk_buff *skb,
                           struct net_device *netdev)
{
    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

static const struct nv_net_ops my_ops = {
    .start_xmit = my_xmit,
};

static int __init my_init(void)
{
    my_drv.name = "my_net_drv";
    my_drv.owner = THIS_MODULE;
    my_drv.count = 1;

    nv_net_driver_register(&my_drv);
    return nv_net_device_register(&my_drv, &my_dev, 0, "nv%d",
                                  &my_ops, priv, NULL, ETH_DATA_LEN);
}
```

`Makefile`：

```makefile
obj-m := my_driver.o
my_driver-y := my_driver.o core/nv_net.o
ccflags-y += -I$(src)/include
```

## 清理

```bash
make clean
```
