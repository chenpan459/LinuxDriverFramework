# nv_char — 字符设备驱动框架

## 目录结构

```
char/
├── driver/          # 内核：core、include、modules/
├── app/             # 用户态：nv_test_echo 等
└── Makefile
```

## 编译与测试

```bash
make -C char
sudo insmod driver/nv_char_echo.ko
app/bin/nv_test_echo "hello"
sudo rmmod nv_char_echo
```

| 应用 | 模块 | 设备 |
|------|------|------|
| nv_test_echo | nv_char_echo.ko | /dev/nv_echo |
| nv_test_demo | nv_char_demo.ko | /dev/nv_demo |
| nv_test_misc | nv_char_misc_echo.ko | /dev/nv_misc |
| nv_test_plat | nv_char_plat.ko | /dev/nv_plat |

`nv_char_selftest.ko` 加载时内核自检，无用户态程序。
