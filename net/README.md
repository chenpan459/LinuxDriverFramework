# nv_net — 网络设备驱动框架

## 目录结构

```
net/
├── driver/
├── app/
└── Makefile
```

## 编译与测试

```bash
make -C net
sudo insmod driver/nv_net_dummy.ko
app/bin/nv_test_net
sudo rmmod nv_net_dummy
```

默认接口名 `nv0`。
