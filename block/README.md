# nv_block — 块设备驱动框架

## 目录结构

```
block/
├── driver/                  # 内核驱动
│   ├── include/nv_block.h
│   ├── core/nv_block.c
│   ├── modules/nv_block_ram.c
│   └── Makefile
├── app/                     # 用户态测试
│   ├── nv_test_ram.c
│   └── Makefile
└── Makefile
```

## 编译

```bash
make -C block              # driver + app
make -C block/driver       # 仅 nv_block_ram.ko
make -C block/app          # 仅 nv_test_ram
```

## 测试

```bash
sudo insmod driver/nv_block_ram.ko
app/bin/nv_test_ram
sudo rmmod nv_block_ram
```

模块参数 `size_mb`（默认 8），设备节点 `/dev/nv_ram`。
