# rtt-ccs811
CCS811 sensor driver for RT-Thread



## 1、介绍

ccs811 软件包是 CCS811 气体传感器的驱动软件包。CCS811 是一款低功耗数字气体传感器，用于检测室内低水平的挥发性有机化合物和二氧化碳浓度，内部集成微控制器单元 (MCU) 和模数转换器（ADC），并提供通过标准 I2C 数字接口获取 CO2 或 TVOC 数据。

CCS811 模块支持 I2C 接口，IIC 地址可配置为 0x5A 或 0X5B。



### 1.1 特性

- 支持静态和动态分配内存。
- 支持 sensor 设备驱动框架。
- 线程安全。



### 1.2 工作模式

| 传感器       | TVOC | eCO2 |
| :----------- | :--- | :--- |
| **通信接口** |      |      |
| I2C          | √    | √    |
| **工作模式** |      |      |
| 轮询         | √    | √    |
| 中断         |      |      |
| FIFO         |      |      |



### 1.3 目录结构

| 名称     | 说明                           |
| -------- | ------------------------------ |
| docs     | 文档目录                       |
| examples | 例子目录（提供两种操作示例）   |
| inc      | 头文件目录                     |
| src      | 源代码目录（提供两种驱动接口） |

驱动源代码提供两种接口，分别是自定义接口，以及 RT-Thread 设备驱动接口（open/read/control/close）。



### 1.4 许可证

ccs811 软件包遵循 Apache license v2.0 许可，详见 `LICENSE` 文件。



### 1.5 依赖

- RT-Thread 4.0+
- 使用动态创建方式需要开启动态内存管理模块
- 使用 sensor 设备接口需要开启 sensor 设备驱动框架模块



## 2、获取 ccs811 软件包

使用 ccs811 package 需要在 RT-Thread 的包管理器中选择它，具体路径如下：

```
RT-Thread online packages --->
    peripheral libraries and drivers --->
        [*] sensors drivers  --->
            [*] CCS811: Digital Gas Sensor for Monitoring Indoor Air Quality..
```

然后让 RT-Thread 的包管理器自动更新，或者使用 `pkgs --update` 命令更新包到 BSP 中。



## 3、使用 ccs811 软件包

### 3.1 版本说明

| 版本   | 说明                                           |
| ------ | ---------------------------------------------- |
| latest | 基本功能测试通过                                     |

目前处于公测阶段，建议开发者使用 latest 版本。



### 3.2 配置选项

- 选择 I2C 地址（`PKG_USING_CCS811_I2C_ADDRESS`）
- 是否使用示例程序（`PKG_USING_CCS811_SAMPLE`）



## 4、API 说明

### 4.1 自定义接口

#### 创建和删除对象

要操作传感器模块，首先需要创建一个传感器对象。

```c
ccs811_device_t ccs811_create(const char *i2c_bus_name);
```

调用这个函数时，会从动态堆内存中分配一个 ccs811_device_t 句柄，并按给定参数初始化。

| 参数            | 描述                         |
| --------------- | ---------------------------- |
| i2c_bus_name    | 设备挂载的 IIC 总线名称      |
| **返回**        | ——                           |
| ccs811_device_t | 创建成功，返回传感器对象句柄 |
| RT_NULL         | 创建失败                     |

对于使用 `ccs811_create()` 创建出来的对象，当不需要使用，或者运行出错时，请使用下面的函数接口将其删除，避免内存泄漏。

```c
void ccs811_delete(ccs811_device_t dev);
```

| **参数**        | **描述**               |
| --------------- | ---------------------- |
| ccs811_device_t | 要删除的传感器对象句柄 |
| **返回**        | ——                     |
| 无              |                        |



#### 初始化对象

如果需要使用静态内存分配，则可调用 `ccs811_init()` 函数。

```c
rt_err_t ccs811_init(struct ccs811_device *dev, const char *i2c_bus_name);
```

使用该函数前需要先创建 ccs811_device 结构体。

| 参数         | 描述                    |
| ------------ | ----------------------- |
| dev          | 传感器对象结构体        |
| i2c_bus_name | 设备挂载的 IIC 总线名称 |
| **返回**     | ——                      |
| RT_EOK       | 初始化成功              |
| -RT_ERROR    | 初始化失败              |



#### 测量数据

测量 TVOC 和 eCO2 浓度值，并将数据保存在传感器对象中。

```c
rt_bool_t ccs811_measure(ccs811_device_t dev);
```

| 参数     | 描述           |
| -------- | -------------- |
| dev      | 传感器对象句柄 |
| **返回** | ——             |
| RT_TRUE  | 读取成功       |
| RT_FALSE | 读取失败       |

由于 CCS811 传感器支持多种模式和测量周期，为成功获取数据，建议在调用 `ccs811_measure()` 前使用 `ccs811_check_ready()` 函数检查传感器是否准备好了。




#### 读取 baseline

```c
rt_uint16_t ccs811_get_baseline(ccs811_device_t dev);
```

| 参数         | 描述           |
| ------------ | -------------- |
| dev          | 传感器对象句柄 |
| **返回**     | ——             |
| 16位的基线值 | 读取成功       |
| 0            | 读取失败       |



#### 设置 baseline

```c
rt_bool_t ccs811_set_baseline(ccs811_device_t dev, rt_uint16_t baseline);
```

| 参数     | 描述                   |
| -------- | ---------------------- |
| dev      | 传感器对象句柄         |
| baseline | 16位的 baseline 设置值 |
| **返回** | ——                     |
| RT_TRUE  | 设置成功               |
| RT_FALSE | 设置失败               |



#### 设置环境数据

```c
rt_bool_t ccs811_set_envdata(ccs811_device_t dev, float temperature, float humidity);
```

| 参数        | 描述             |
| ----------- | ---------------- |
| dev         | 传感器对象句柄   |
| temperature | 当前环境的温度值 |
| humidity    | 当前环境的湿度值 |
| **返回**    | ——               |
| RT_TRUE     | 设置成功         |
| RT_FALSE    | 设置失败         |

在测量过程中定期设置环境温度和湿度值，有利于获取更准确的数据。



#### 设置测量周期

```c
rt_bool_t  ccs811_set_measure_cycle(ccs811_device_t dev, ccs811_cycle_t cycle);
```

| 参数     | 描述           |
| -------- | -------------- |
| dev      | 传感器对象句柄 |
| cycle    | 测量周期       |
| **返回** | ——             |
| RT_TRUE  | 设置成功       |
| RT_FALSE | 设置失败       |

测量周期包括 250ms、1s、10s 和 60s，具体可配置项如下：

```c
typedef enum
{
    CCS811_CLOSED,
    CCS811_CYCLE_1S,
    CCS811_CYCLE_10S,
    CCS811_CYCLE_60S,
    CCS811_CYCLE_250MS

} ccs811_cycle_t;
```



#### 设置工作模式

```c
rt_bool_t ccs811_set_measure_mode(ccs811_device_t dev, 
                                  rt_uint8_t thresh, 
                                  rt_uint8_t interrupt, 
                                  ccs811_mode_t mode);
```

| 参数      | 描述                         |
| --------- | ---------------------------- |
| dev       | 传感器对象句柄               |
| thresh    | 0：不检测阈值，1：检测阈值   |
| interrupt | 0：不使能中断，1：使能中断   |
| mode      | 工作模式（就是设置测量周期） |
| **返回**  | ——                           |
| RT_TRUE   | 设置成功                     |
| RT_FALSE  | 设置失败                     |

工作模式可选项如下：

```c
typedef enum
{
    CCS811_MODE_0,
    CCS811_MODE_1,
    CCS811_MODE_2,
    CCS811_MODE_3,
    CCS811_MODE_4

} ccs811_mode_t;
```



#### 读取工作模式

```c
rt_uint8_t ccs811_get_measure_mode(ccs811_device_t dev);
```

| 参数               | 描述           |
| ------------------ | -------------- |
| dev                | 传感器对象句柄 |
| **返回**           | ——             |
| MEAS_MODE 寄存器值 | 读取成功       |
| 0xFF               | 读取失败       |



#### 设置报警阈值

```c
rt_bool_t ccs811_set_thresholds(ccs811_device_t dev, 
                                rt_uint16_t low_to_med, 
                                rt_uint16_t med_to_high);
```

| 参数        | 描述                                 |
| ----------- | ------------------------------------ |
| dev         | 传感器对象句柄                       |
| low_to_med  | 低范围到中范围的阈值，默认为 1500ppm |
| med_to_high | 中范围到高范围的阈值，默认为 2500ppm |
| **返回**    | ——                                   |
| RT_TRUE     | 设置成功                             |
| RT_FALSE    | 设置失败                             |

注意：阈值设置只针对 CO~2~ 气体浓度。



### 4.2 Sensor 接口

ccs811 软件包已对接 sensor 驱动框架，操作传感器模块之前，只需调用下面接口注册传感器设备即可。

```c
rt_err_t rt_hw_ccs811_init(const char *name, struct rt_sensor_config *cfg);
```

| 参数      | 描述            |
| --------- | --------------- |
| name      | 传感器设备名称  |
| cfg       | sensor 配置信息 |
| **返回**  | ——              |
| RT_EOK    | 创建成功        |
| -RT_ERROR | 创建失败        |



#### 初始化示例

```c
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "ccs811.h"

#define CCS811_I2C_BUS_NAME       "i2c1"

static int rt_hw_ccs811_port(void)
{
    struct rt_sensor_config cfg;
    
    cfg.intf.type = RT_SENSOR_INTF_I2C;
    cfg.intf.dev_name = CCS811_I2C_BUS_NAME;
    rt_hw_ccs811_init("cs8", &cfg);
    
    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_ccs811_port);
```



#### 传感器测试

将上述 sensor 初始化示例代码加入工程，编译下载后即可进行测试。（注意：需要先配置好 i2c1 总线，并添加 sensor 组件）

**检查传感器是否初始化成功**

```shell
msh >list_device
device           type         ref count
-------- -------------------- ----------
tvoc_cs8 Sensor Device        0
eco2_cs8 Sensor Device        0
```

**查看 CCS811 信息**

```shell
msh >sensor probe tvoc_cs8
[4774993] I/sensor.cmd: device id: 0x81!

msh >sensor info
vendor    :AMS
model     :ccs811
unit      :ppb
range_max :32768
range_min :0
period_min:250ms
fifo_max  :1
```

**读取 TVOC 数据**

```shell
msh >sensor read
[4794468] I/sensor.cmd: num:  0, tvoc:  184 ppb, timestamp:4794468
[4794586] I/sensor.cmd: num:  1, tvoc:  184 ppb, timestamp:4794586
[4794704] I/sensor.cmd: num:  2, tvoc:  184 ppb, timestamp:4794704
[4794822] I/sensor.cmd: num:  3, tvoc:  184 ppb, timestamp:4794822
[4794940] I/sensor.cmd: num:  4, tvoc:  184 ppb, timestamp:4794940
```

**读取 CO2 数据**

```shell
msh >sensor read
[4957632] I/sensor.cmd: num:  0, eco2:  871 ppm, timestamp:4957632
[4957850] I/sensor.cmd: num:  1, eco2:  865 ppm, timestamp:4957850
[4957968] I/sensor.cmd: num:  2, eco2:  865 ppm, timestamp:4957968
[4958086] I/sensor.cmd: num:  3, eco2:  871 ppm, timestamp:4958086
[4958303] I/sensor.cmd: num:  4, eco2:  871 ppm, timestamp:4958303
```



## 5、注意事项

1. 为传感器对象提供静态创建和动态创建两种方式，如果使用动态创建，请记得在使用完毕释放对应的内存空间。
2. 由于 CCS811 模块包含一个 TVOC 传感器和一个 eCO2 传感器，因此在 sensor 框架中会注册两个设备，内部提供1位 FIFO 缓存进行同步，缓存空间在调用 `rt_device_open` 函数时创建，因此 read 之前务必确保两个设备都开启成功。
3. 由于使用 I2C 接口进行操作，因此注册时需指定具体的 I2C 总线名称，对应的句柄存放在 user_data 中。
4. CCS811 传感器需要预热，预热时间小于15秒，此前的数据一直是 TVOC 为 0，eCO2 为 400。
5. 数据手册建议在第一次使用传感器时，先运行48小时。
6. CCS811 传感器会自动校准基线，但是这个过程非常缓慢。数据手册对基线校准的建议：在运行传感器的第一周，建议每24小时保存一个新的基线，运行1周后，可以每1-28天保存一次。
7. 如需使用中断功能，请将传感器的 INT 引脚连接到主控板相应的中断引脚。



## 6、相关文档

见 docs 目录。



## 7、联系方式

- 维护：luhuadong@163.com
- 主页：<https://github.com/luhuadong/rtt-ccs811>
