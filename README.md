# ec-round2-2602123W08

CVIR 电控二轮 · 四自由度舵机机械臂控制固件

- 主控：STM32F103C8T6（Blue Pill，64KB Flash / 20KB RAM）
- 工具链：STM32CubeMX + CMake + Ninja + arm-none-eabi-gcc
- 执行器：4 × MG90S 金属齿轮舵机（50Hz PWM，500–2500µs）
- 传感器：MPU6050（I²C1，400kHz）
- 上位机通信：USB CDC 虚拟串口，A5 帧协议

## 硬件接线

| 功能 | 引脚 | 说明 |
|---|---|---|
| 舵机 1（底座） | PA0 / TIM2_CH1 | PWM 50Hz |
| 舵机 2（大臂） | PA1 / TIM2_CH2 | PWM 50Hz |
| 舵机 3（小臂） | PA2 / TIM2_CH3 | PWM 50Hz |
| 舵机 4（夹爪） | PA3 / TIM2_CH4 | PWM 50Hz |
| MPU6050 SCL | PB6 / I2C1_SCL | 400kHz，上拉 4.7k |
| MPU6050 SDA | PB7 / I2C1_SDA | 400kHz，上拉 4.7k |
| 调试串口 TX | PA9 / USART1_TX | 115200-8-N-1 |
| 调试串口 RX | PA10 / USART1_RX | 115200-8-N-1 |
| USB D− / D+ | PA11 / PA12 | 全速设备，48MHz |
| 板载 LED | PC13 | 低电平点亮 |
| 按键 | PB12 | 内部上拉，按下为低 |

> 舵机电源必须用独立 5V/5A 适配器（MG90S 堵转电流约 1.5–2A/个，四路同时动作峰值可达 6–8A），
> 电源端并联 1000µF 电解电容 + 100nF 陶瓷电容，**不要**从板载 3.3V/5V 取电。
> 舵机地与主控地必须共地。

## 构建

```bash
cmake --preset Debug
cmake --build --preset Debug
```

产物位于 `build/Debug/`。

## 烧录

待补充（ST-Link / DAP-Link 具体命令）。

## 目录结构

```
Core/          应用与 HAL 初始化代码
Drivers/       CMSIS 与 HAL 驱动
cmake/         工具链与链接脚本
build/         构建输出（不提交）
```

## 进度

| 任务 | 状态 |
|---|---|
| T0 板级自检 | 进行中 |
| T1 单轴 PWM | 未开始 |
| T2 四轴插补 | 未开始 |
| T3 标定与限位 | 未开始 |
| T4 MPU6050 | 未开始 |
| T5a CDC 上报 | 未开始 |