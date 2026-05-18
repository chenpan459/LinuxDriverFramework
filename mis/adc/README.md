# nv_adc — ADC 驱动框架（IIO）

基于 Linux **Industrial I/O (IIO)** 子系统，封装 `iio_device_alloc` / `iio_device_register` 与 `read_raw` / `write_raw` 分发。

## 目录结构

```
adc/
├── driver/
│   ├── include/nv_adc.h
│   ├── core/nv_adc.c
│   ├── modules/nv_adc_demo.c
│   └── Makefile
├── app/
│   ├── nv_test_adc.c
│   ├── nv_test_adc_sysfs.sh
│   └── Makefile
└── Makefile
```

## 框架流程

```
nv_adc_register()
  └─ iio_device_alloc() + iio_device_register()
        └─ iio_info.read_raw / write_raw
              └─ nv_adc_ops (驱动实现)
```

## 编译

```bash
make -C adc
```

## 测试（软件 ADC demo）

```bash
sudo insmod adc/driver/nv_adc_demo.ko
# /sys/bus/iio/devices/iio:deviceN/in_voltage0_raw ...
adc/app/bin/nv_test_adc

# 或
sudo adc/app/bin/nv_test_adc_sysfs
```

默认通道初值：ch0=1000, ch1=2000, ch2=3000, ch3=4000（毫伏数值，demo 中为整数 raw）。

## API 摘要

| 接口 | 说明 |
|------|------|
| `nv_adc_register` | 注册 IIO ADC 设备 |
| `nv_adc_unregister` | 注销 |
| `NV_ADC_CHAN_VOLTAGE(n)` | 定义电压通道宏 |

### nv_adc_ops

| 回调 | 说明 |
|------|------|
| `read_raw` | 必需，读取 `IIO_CHAN_INFO_RAW` |
| `write_raw` | 可选，写入 raw 值 |

## 编写新驱动

```c
static const struct iio_chan_spec my_channels[] = {
	NV_ADC_CHAN_VOLTAGE(0),
};

static int my_read_raw(struct nv_adc_dev *dev, struct iio_chan_spec const *chan,
		       int *val, int *val2, long mask)
{
	*val = sample_from_hw();
	return IIO_VAL_INT;
}

nv_adc_register(parent, &my_adc, "my_adc", my_channels, ARRAY_SIZE(my_channels));
```
