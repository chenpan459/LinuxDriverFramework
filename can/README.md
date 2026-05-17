# nv_can — CAN 总线（SocketCAN）驱动框架

基于 Linux `can_dev` / SocketCAN，封装 `alloc_candev`、`register_candev` 与 `can_priv` 回调分发。

## 目录结构

```
can/
├── driver/
│   ├── include/nv_can.h
│   ├── core/nv_can.c
│   ├── modules/nv_can_demo.c
│   └── Makefile
├── app/
│   ├── nv_test_can.c
│   ├── nv_test_can_sysfs.sh
│   └── Makefile
└── Makefile
```

## 框架流程

```
nv_can_netdev_register()
  └─ alloc_candev() + register_candev()
        └─ can_priv.do_set_mode / do_set_bittiming
        └─ ndo_start_xmit → nv_can_ops.start_xmit
```

## 编译

```bash
make -C can
```

用户态测试需要内核头文件中的 `linux/can.h`（Makefile 已添加 include 路径）。

## 测试（虚拟回环 demo）

```bash
sudo insmod can/driver/nv_can_demo.ko
sudo ip link set nvcan0 up type can bitrate 1000000
can/app/bin/nv_test_can

# 或一键
sudo can/app/bin/nv_test_can_sysfs
```

## API 摘要

| 接口 | 说明 |
|------|------|
| `nv_can_netdev_register` | 注册 CAN 网络设备 |
| `nv_can_netdev_unregister` | 注销 |
| `nv_can_rx_frame` / `nv_can_rx_skb` | 向协议栈注入接收帧 |
| `nv_can_cpriv` | 获取 `struct can_priv` |

### nv_can_ops

| 回调 | 说明 |
|------|------|
| `start_xmit` | 必需，发送处理 |
| `do_set_mode` | START/STOP/SLEEP |
| `do_set_bittiming` | 位时序配置 |
| `do_get_state` | 可选，总线状态 |

## 编写新驱动

```c
static netdev_tx_t my_xmit(struct nv_can_dev *dev, struct sk_buff *skb)
{
	/* 发送到 CAN 控制器 */
	return NETDEV_TX_OK;
}

static const struct nv_can_ops my_ops = {
	.do_set_mode = my_set_mode,
	.start_xmit = my_xmit,
};

nv_can_netdev_register(&my_dev, "mycan0", &my_ops, priv, &my_btc, 0);
```
