# RS232驱动包

## 1.简介

**rs232** rs232接口通信驱动包。
使用说明：
本软件包发送需要DMA支持。
接收需要中断接收（每次一个字节）。
目的是收发数据都是非阻塞型的。
原理是利用DMA一次完整发送数据。利用接收间隔，超时判定为接收完成。(超过大小限制的数据丢弃最后部分)

目前在ST系列下验证过。

### 1.1目录结构

`rs232` 软件包目录结构如下所示：

``` 
rs232
├───inc                         // 头文件目录
│   └───rs232.h                 // API 接口头文件
├───src                         // 源码目录
│   |   rs232.c                 // 主模块
│   |   rs232_sample_.c    		// 示例
│   license                     // 软件包许可证
│   readme.md                   // 软件包使用说明
└───SConscript                  // RT-Thread 默认的构建脚本
```

### 1.2许可证

rs232 package 遵循 LGPLv2.1 许可，详见 `LICENSE` 文件。

### 1.3依赖

- RT_Thread 4.0
- serial

## 2.使用

### 2.1接口函数说明

#### rs232_inst_t * rs232_create(char *serial, int baudrate, int parity);
- 功能 ：动态创建rs232实例
- 参数 ：serial--串口设备名称
- 参数 ：baudrate--串口波特率
- 参数 ：parity--串口检验位
- 返回 ：成功返回实例指针，失败返回NULL

#### int rs232_destory(rs232_inst_t * hinst);
- 功能 ：销毁rs232实例
- 参数 ：hinst--rs232实例指针
- 返回 ：0--成功,其它--失败

#### int rs232_config(rs232_inst_t * hinst, int baudrate, int databits, int parity, int stopbits);
- 功能 ：配置rs232通信参数
- 参数 ：hinst--rs232实例指针
- 参数 ：baudrate--通信波特率
- 参数 ：databits--数据位数, 5~8
- 参数 ：parity--检验位, 0~2, 0--无校验, 1--奇校验, 2--偶校验
- 参数 ：stopbits--停止位, 0~1, 0--1个停止位, 1--2个停止位
- 返回 ：0--成功，其它--错误

#### int rs232_set_byte_tmo(rs232_inst_t * hinst, int tmo_ms);
- 功能 ：设置rs232接收字节间隔超时时间
- 参数 ：hinst--rs232实例指针
- 参数 ：tmo_ms--超时时间,单位ms
- 返回 ：0--成功，其它--错误

#### int rs232_connect(rs232_inst_t * hinst);
- 功能 ：打开rs232连接
- 参数 ：hinst--rs232实例指针
- 返回 ：0--成功，其它--错误

#### int rs232_disconn(rs232_inst_t * hinst);
- 功能 ：关闭rs232连接
- 参数 ：hinst--rs232实例指针
- 返回 ：0--成功，其它--错误

#### int rs232_recv(rs232_inst_t * hinst, void *buf, int size);
- 功能 ：从rs232接收数据
- 参数 ：hinst--rs232实例指针
- 参数 ：buf--接收数据缓冲区指针
- 参数 ：size--缓冲区尺寸
- 返回 ：>=0--接收到的数据长度，<0--错误

#### int rs232_send(rs232_inst_t * hinst, void *buf, int size);
- 功能 ：向rs232发送数据
- 参数 ：hinst--rs232实例指针
- 参数 ：buf--发送数据缓冲区指针
- 参数 ：size--发送数据长度
- 返回 ：>=0--发送的数据长度，<0--错误

### 2.2获取组件

