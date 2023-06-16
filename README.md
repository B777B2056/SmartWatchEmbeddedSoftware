# 简易智能手表
## 1 包含功能
* OLED
* 计步
* 心率采集
* 血氧采集
* 时间与日期显示
* 手机来电提示，接听/拒绝
* [安卓APP上位机](https://github.com/B777B2056/SmartWatchMobileAPP)

## 2 实物效果
TODO

## 3 硬件设计
* MCU：1xSTM32F103CBT6
* 1x0.96寸OLED（I2C）
* 计步：1xADXL345B（SPI）
* 心率&血氧：1xMAX30102（I2C）
* 通讯：1xHC-06（UART）
* 2x微动开关

## 4 软件设计
* 基于FreeRtos的双Task：Frontend Task与 Backend Task
* Frontend Task：OLED显示+来电监测
* Backend Task：芯片数据采集
