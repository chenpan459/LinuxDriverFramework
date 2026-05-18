# nv_uart — UART 串口驱动框架

基于 Linux `serial_core`，封装 `uart_register_driver` / `uart_add_one_port` 与 `uart_ops` 分发。

## 目录结构

```
uart/
├── driver/
│   ├── include/nv_uart.h
│   ├── core/nv_uart.c
│   ├── modules/nv_uart_demo.c
│   └── Makefile
├── app/
│   ├── nv_test_uart.c
│   ├── nv_test_uart_sysfs.sh
│   └── Makefile
└── Makefile
```

## 框架流程

```
nv_uart_driver_register()     # 注册 tty 驱动 (如 ttyNV)
  └─ nv_uart_port_add()       # 添加 uart_port
        └─ uart_add_one_port()
              └─ nv_uart_port_ops (start_tx, ...)
```

## 编译

```bash
make -C uart
```

## 测试（回环 demo）

```bash
sudo insmod uart/driver/nv_uart_demo.ko
# 设备节点 /dev/ttyNV0
uart/app/bin/nv_test_uart
sudo rmmod nv_uart_demo

# 或一键
sudo uart/app/bin/nv_test_uart_sysfs
```

## API 摘要

| 接口 | 说明 |
|------|------|
| `nv_uart_driver_register` | 注册 `uart_driver` |
| `nv_uart_port_add` / `remove` | 添加/移除端口 |
| `nv_uart_push_rx` | 向 TTY 层推送接收数据 |
| `nv_uart_write_wakeup` | 唤醒写等待 |

### nv_uart_port_ops

| 回调 | 说明 |
|------|------|
| `start_tx` | 开始发送（demo 在此做回环） |
| `startup` / `shutdown` | 端口启停 |
| `set_termios` | 波特率等（框架已处理默认项） |
| `tx_empty` / `stop_tx` | 发送状态 |

## 编写新驱动

```c
static struct nv_uart_driver my_drv;
static struct nv_uart_port my_port;

static void my_start_tx(struct nv_uart_port *np)
{
	/* 从硬件 FIFO 发送 */
}

static const struct nv_uart_port_ops my_ops = {
	.start_tx = my_start_tx,
};

nv_uart_driver_register(&my_drv);
nv_uart_port_add(&my_drv, &my_port, 0);
```
